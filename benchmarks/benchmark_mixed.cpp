#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <initializer_list>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "b_tree.hpp"
#include "set_adapter.hpp"
#include "sorted_vector.hpp"
#include "threaded_avl_tree.hpp"

namespace {

// Порядок ключей и вид запроса задаются независимо от доли вставок.
enum class Pattern { Increasing, Decreasing, Random };
enum class Query { Select, CountLess, RangeNarrow, RangeWide, InsertOnly };

struct Configuration
{
    std::size_t operations;
    int insert_percent;
    Pattern pattern;
    Query query;
    unsigned seed;
    bool duplicates = false;
};

struct Command
{
    bool insertion = false;
    int key = 0;
    int right = 0;
    std::size_t index = 0;
    std::uint64_t expected = 0;
};

struct Workload
{
    std::vector<Command> commands;
    std::vector<int> ordered;
    std::size_t insertions = 0;
    std::uint64_t checksum = 0;
};

// Возвращает короткое имя паттерна для таблицы результатов.
const char* pattern_name(Pattern pattern)
{
    switch (pattern)
    {
        case Pattern::Increasing: return "Increasing";
        case Pattern::Decreasing: return "Decreasing";
        case Pattern::Random: return "Random";
    }
    throw std::invalid_argument("Неизвестный паттерн");
}

// Возвращает имя семейства запросов; чистые вставки не дублируются.
const char* query_name(Query query)
{
    switch (query)
    {
        case Query::Select: return "Insert_Select";
        case Query::CountLess: return "Insert_CountLess";
        case Query::RangeNarrow: return "Insert_RangeNarrow";
        case Query::RangeWide: return "Insert_RangeWide";
        case Query::InsertOnly: return "InsertOnly";
    }
    throw std::invalid_argument("Неизвестный запрос");
}

// Создаёт ключи вставок. В отдельной серии каждый ключ повторяется дважды.
std::vector<int> make_keys(const Configuration& config, std::size_t insertions)
{
    const std::size_t unique = config.duplicates ? (insertions + 1) / 2 : insertions;
    std::vector<int> keys(insertions);
    for (std::size_t i = 0; i < insertions; ++i)
    {
        keys[i] = static_cast<int>(config.duplicates ? i / 2 + 1 : i + 1);
    }
    if (config.pattern == Pattern::Decreasing)
    {
        for (int& key : keys)
        {
            key = static_cast<int>(unique) + 1 - key;
        }
    }
    else if (config.pattern == Pattern::Random)
    {
        std::mt19937 generator(config.seed);
        std::shuffle(keys.begin(), keys.end(), generator);
    }
    return keys;
}

// Задаёт точную долю вставок; первая команда всегда вставляет первый ключ.
std::vector<int> make_operation_order(const Configuration& config, std::size_t insertions)
{
    std::vector<int> order(config.operations, 0);
    std::fill_n(order.begin(), insertions, 1);
    std::mt19937 generator(config.seed + 1);
    std::shuffle(order.begin() + 1, order.end(), generator);
    return order;
}

// Готовит команды и эталонные ответы по текущему отсортированному множеству.
// Весь этот код выполняется до включения таймера Google Benchmark.
Workload make_workload(const Configuration& config)
{
    if (config.operations == 0 || config.operations > 1000000 ||
        config.insert_percent < 1 || config.insert_percent > 100 ||
        (config.operations * config.insert_percent) % 100 != 0)
    {
        throw std::invalid_argument("Недопустимое число команд или доля вставок");
    }

    Workload result;
    result.insertions = config.operations * config.insert_percent / 100;
    const auto keys = make_keys(config, result.insertions);
    const auto order = make_operation_order(config, result.insertions);
    result.commands.reserve(config.operations);
    result.ordered.reserve(result.insertions);
    std::mt19937 generator(config.seed + 2);
    std::size_t next_key = 0;

    for (int insertion : order)
    {
        Command command;
        command.insertion = insertion != 0;
        if (command.insertion)
        {
            command.key = keys[next_key++];
            const auto position = std::lower_bound(
                result.ordered.begin(), result.ordered.end(), command.key);
            if (position == result.ordered.end() || *position != command.key)
            {
                result.ordered.insert(position, command.key);
            }
        }
        else
        {
            const std::size_t size = result.ordered.size();
            std::uniform_int_distribution<std::size_t> position(0, size - 1);
            if (config.query == Query::Select)
            {
                command.index = position(generator) + 1;
                command.expected = result.ordered[command.index - 1];
            }
            else if (config.query == Query::CountLess)
            {
                const std::size_t index = position(generator);
                command.key = result.ordered[index];
                command.expected = index;
            }
            else if (config.query == Query::RangeNarrow ||
                     config.query == Query::RangeWide)
            {
                const std::size_t width = size == 1 ? 0 :
                    (config.query == Query::RangeNarrow ? 1 : size / 2);
                std::uniform_int_distribution<std::size_t> start(0, size - width - 1);
                const std::size_t left = start(generator);
                command.key = result.ordered[left];
                command.right = result.ordered[left + width];
                command.expected = width;
            }
            else
            {
                throw std::logic_error("Запрос в сценарии только вставок");
            }
            result.checksum += command.expected;
        }
        result.commands.push_back(command);
    }
    return result;
}

// Создаёт пустой контейнер; степень передаётся только B-дереву.
template <typename Container>
Container make_container(std::size_t degree)
{
    if constexpr (std::is_same_v<Container, search_tools::B_Tree<int>>)
    {
        return Container(degree);
    }
    else
    {
        return Container();
    }
}

// Выполняет один запрос. Отсутствующий select считается ошибкой, а не нулём.
template <typename Container>
std::uint64_t execute_query(const Container& container, const Command& command, Query query)
{
    switch (query)
    {
        case Query::Select:
        {
            const auto value = container.select(command.index);
            if (!value)
            {
                throw std::logic_error("select вернул nullopt для допустимого индекса");
            }
            return static_cast<std::uint64_t>(*value);
        }
        case Query::CountLess: return container.count_less(command.key);
        case Query::RangeNarrow:
        case Query::RangeWide: return container.range_count(command.key, command.right);
        case Query::InsertOnly: break;
    }
    throw std::logic_error("Неизвестный запрос");
}

// До замера сверяет каждый ответ и изменение размера с независимым эталоном.
template <typename Container>
void validate_workload(const Workload& workload, Query query, std::size_t degree)
{
    Container container = make_container<Container>(degree);
    std::vector<int> ordered;
    for (const Command& command : workload.commands)
    {
        if (command.insertion)
        {
            container.insert(command.key);
            const auto position = std::lower_bound(ordered.begin(), ordered.end(), command.key);
            if (position == ordered.end() || *position != command.key)
            {
                ordered.insert(position, command.key);
            }
            if (container.size() != ordered.size())
            {
                throw std::logic_error("Неверный размер после вставки");
            }
        }
        else if (execute_query(container, command, query) != command.expected)
        {
            throw std::logic_error("Ответ контейнера не совпал с эталоном");
        }
    }
    for (std::size_t i = 0; i < ordered.size(); ++i)
    {
        const auto value = container.select(i + 1);
        if (!value || *value != ordered[i])
        {
            throw std::logic_error("Неверный итоговый набор ключей");
        }
    }
}

// Измеряет полный проход команд. Создание и удаление контейнера исключены.
// Таймер включает диспетчеризацию команд, вызовы методов и контрольную сумму.
template <typename Container>
void benchmark_workload(benchmark::State& state, const Configuration& config,
                        std::size_t degree, Workload& workload)
{
    try
    {
        if (workload.commands.empty())
        {
            workload = make_workload(config);
        }
        validate_workload<Container>(workload, config.query, degree);
    }
    catch (const std::exception& error)
    {
        state.SkipWithError(error.what());
        return;
    }

    for (auto iteration : state)
    {
        static_cast<void>(iteration);
        state.PauseTiming();
        bool valid = true;
        {
            Container container = make_container<Container>(degree);
            std::uint64_t checksum = 0;
            state.ResumeTiming();

            for (const Command& command : workload.commands)
            {
                if (command.insertion)
                {
                    container.insert(command.key);
                }
                else
                {
                    checksum += execute_query(container, command, config.query);
                }
            }
            benchmark::DoNotOptimize(checksum);
            benchmark::DoNotOptimize(container);
            benchmark::ClobberMemory();

            state.PauseTiming();
            valid = checksum == workload.checksum && container.size() == workload.ordered.size();
        }
        // Контейнер уже уничтожен при остановленном таймере.
        state.ResumeTiming();
        if (!valid)
        {
            state.SkipWithError("Не совпала контрольная сумма или итоговый размер");
            break;
        }
    }

    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(config.operations));
    state.counters["seconds_per_command"] = benchmark::Counter(
        static_cast<double>(state.iterations()) * config.operations,
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
    state.counters["Q"] = static_cast<double>(config.operations);
    state.counters["insert_percent"] = config.insert_percent;
    state.counters["insert_attempts"] = static_cast<double>(workload.insertions);
    state.counters["queries"] = static_cast<double>(config.operations - workload.insertions);
    state.counters["final_U"] = static_cast<double>(workload.ordered.size());
    state.counters["seed"] = config.seed;
    state.counters["t"] = static_cast<double>(degree);
    state.counters["duplicate_attempts"] = static_cast<double>(workload.insertions - workload.ordered.size());
    state.SetLabel("time_per_sequence; initial_U=0; checksum=" + std::to_string(workload.checksum));
}

// Регистрирует один класс или проверяет его без проведения измерений.
template <typename Container>
void register_container(const std::string& name, const Configuration& config,
                        std::size_t degree, const std::shared_ptr<Workload>& workload,
                        bool check_only)
{
    if (check_only)
    {
        if (workload->commands.empty())
        {
            *workload = make_workload(config);
        }
        validate_workload<Container>(*workload, config.query, degree);
        return;
    }
    benchmark::RegisterBenchmark(name.c_str(),
        [config, degree, workload](benchmark::State& state)
        {
            benchmark_workload<Container>(state, config, degree, *workload);
        })->Unit(benchmark::kMillisecond)->UseRealTime();
}

// Подключает все четыре реализации и шесть степеней B-дерева к одной нагрузке.
void register_configuration(const std::string& suite, const Configuration& config, bool check_only)
{
    const std::string name = suite + "/" + pattern_name(config.pattern) + "/" +
        query_name(config.query) + "/Q:" + std::to_string(config.operations) +
        "/P:" + std::to_string(config.insert_percent) + "/seed:" + std::to_string(config.seed);
    auto workload = std::make_shared<Workload>();
    register_container<search_tools::Sorted_Vector<int>>(
        name + "/Sorted_Vector", config, 0, workload, check_only);
    register_container<search_tools::Set_Adapter<int>>(
        name + "/Set_Adapter", config, 0, workload, check_only);
    register_container<search_tools::AVL_Tree<int>>(
        name + "/AVL_Tree", config, 0, workload, check_only);
    for (std::size_t degree : {2u, 4u, 8u, 16u, 32u, 64u})
    {
        register_container<search_tools::B_Tree<int>>(
            name + "/B_Tree/t:" + std::to_string(degree), config, degree, workload, check_only);
    }
}

// Выбирает основную или дополнительную серию. Дополнительные серии запускаются явно.
std::size_t register_suite(const std::string& suite, bool check_only)
{
    std::vector<std::size_t> sizes = {10000};
    std::vector<int> percents = {10, 25, 50, 75, 90, 100};
    std::vector<unsigned> seeds = {42};
    bool duplicates = false;
    if (suite == "pilot")
    {
        sizes = {1000};
    }
    else if (suite == "scale")
    {
        sizes = {1000, 10000, 50000};
    }
    else if (suite == "seeds")
    {
        seeds = {42, 123, 2026};
    }
    else if (suite == "duplicates")
    {
        duplicates = true;
    }
    else if (suite == "refine")
    {
        percents = {30, 35, 40, 45};
    }
    else if (suite != "main")
    {
        throw std::invalid_argument("Серии: pilot, main, scale, seeds, duplicates, refine");
    }

    std::size_t count = 0;
    for (std::size_t size : sizes)
    {
        for (unsigned seed : seeds)
        {
            for (Pattern pattern : {Pattern::Increasing, Pattern::Decreasing, Pattern::Random})
            {
                for (int percent : percents)
                {
                    for (Query query : {Query::Select, Query::CountLess, Query::RangeNarrow, Query::RangeWide})
                    {
                        Configuration config{size, percent, pattern,
                            percent == 100 ? Query::InsertOnly : query, seed, duplicates};
                        register_configuration(suite, config, check_only);
                        count += 9;
                        if (percent == 100)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }
    return count;
}

// Добавляет значение параметра по умолчанию, сохраняя явные настройки пользователя.
void add_default(std::vector<std::string>& arguments, const std::string& flag, const std::string& value)
{
    for (const auto& argument : arguments)
    {
        if (argument == flag || argument.starts_with(flag + "="))
        {
            return;
        }
    }
    arguments.push_back(flag + "=" + value);
}

} // namespace

// Разбирает параметры серии и передаёт стандартные флаги измерений Google Benchmark.
int main(int argc, char** argv)
{
    try
    {
        std::string suite = "main";
        bool check_only = false;
        std::vector<std::string> arguments = {argv[0]};

        for (int i = 1; i < argc; ++i)
        {
            const std::string argument = argv[i];
            if (argument.starts_with("--mixed_suite="))
            {
                suite = argument.substr(std::string("--mixed_suite=").size());
            }
            else if (argument == "--mixed_check_only")
            {
                check_only = true;
            }
            else
            {
                arguments.push_back(argument);
            }
        }

        if (check_only)
        {
            const auto count = register_suite(suite, true);
            std::cout << "Проверено конфигураций: " << count << '\n';
            return 0;
        }

        add_default(arguments, "--benchmark_repetitions", "7");
        add_default(arguments, "--benchmark_min_time", "0.05");
        add_default(arguments, "--benchmark_enable_random_interleaving", "true");

        std::vector<char*> pointers;
        for (auto& argument : arguments)
        {
            pointers.push_back(argument.data());
        }

        int benchmark_argc = static_cast<int>(pointers.size());
        pointers.push_back(nullptr);
        benchmark::Initialize(&benchmark_argc, pointers.data());

        if (benchmark::ReportUnrecognizedArguments(benchmark_argc, pointers.data()))
        {
            return 1;
        }

        register_suite(suite, false);

        benchmark::AddCustomContext("mixed_suite", suite);
        benchmark::AddCustomContext("measurement", "empty container; time per Q commands; destruction excluded");
        benchmark::RunSpecifiedBenchmarks();
        benchmark::Shutdown();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
