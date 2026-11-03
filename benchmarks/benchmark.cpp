#include "benchmark.hpp"

#include <iostream>
#include <chrono>
#include <charconv>
#include <cstddef>
#include <string>
#include <system_error>
#include <algorithm>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <optional>
#include <cstdlib>
#include <utility>

#include "b_tree.hpp"
#include "set_adapter.hpp"
#include "sorted_vector.hpp"
#include "threaded_avl_tree.hpp"

namespace {

void print_measurement(const char* container_name, const char* sequence_name,
                       const char* method, std::size_t insertions,
                       std::size_t unique_count, std::size_t operations,
                       std::optional<std::chrono::steady_clock::duration> elapsed,
                       std::optional<std::uint64_t> checksum = std::nullopt,
                       const char* case_name = nullptr)
{
    std::cout << container_name << " | " << sequence_name << " | " << method;
    if (case_name)
    {
        std::cout << " | " << case_name;
    }
    std::cout << " | вставок = " << insertions << " | уникальных = " << unique_count
              << " | операций = " << operations;

    if (elapsed)
    {
        const double total_ms = std::chrono::duration<double, std::milli>(*elapsed).count();
        const double average_ns = (operations == 0) ? 0 : std::chrono::duration<double, std::nano>(*elapsed).count() / static_cast<double>(operations);

        std::cout << " | всего = " << total_ms << " мс | среднее = " << average_ns << " нс";
    }
    else
    {
        std::cout << " | недостаточно элементов";
    }

    if (checksum)
    {
        std::cout << " | контрольная сумма = " << *checksum;
    }
    std::cout << '\n';
}

template <typename Container>
void benchmark_container(Container& container, const char* container_name,
                         const char* sequence_name, const std::vector<int>& data,
                         const std::vector<int>& ordered,
                         const std::vector<std::size_t>& ranks,
                         const std::vector<std::pair<int, int>>& narrow_ranges,
                         const std::vector<std::pair<int, int>>& wide_ranges)
{
    using Clock = std::chrono::steady_clock;
    const std::size_t unique_count = ordered.size();

    if (!container.empty())
    {
        throw std::invalid_argument("Перед insert контейнер должен быть пустым");
    }

    const auto start_insert = Clock::now();

    for (int key : data)
    {
        container.insert(key);
    }

    const auto finish_insert = Clock::now();

    if (container.size() != unique_count)
    {
        throw std::runtime_error(std::string(container_name) + ": неверное число уникальных ключей");
    }

    std::uint64_t select_checksum = 0;
    const auto start_select = Clock::now();

    for (std::size_t rank : ranks)
    {
        const auto value = container.select(rank);
        select_checksum += static_cast<std::uint64_t>(value.value_or(0));
    }

    const auto finish_select = Clock::now();

    std::uint64_t count_checksum = 0;
    const auto start_count = Clock::now();

    for (int key : data)
    {
        count_checksum += container.count_less(key);
    }

    const auto finish_count = Clock::now();

    std::uint64_t narrow_checksum = 0;
    const auto start_narrow = Clock::now();

    for (const auto& [left, right] : narrow_ranges)
    {
        narrow_checksum += container.range_count(left, right);
    }

    const auto finish_narrow = Clock::now();

    std::uint64_t wide_checksum = 0;
    const auto start_wide = Clock::now();

    for (const auto& [left, right] : wide_ranges)
    {
        wide_checksum += container.range_count(left, right);
    }

    const auto finish_wide = Clock::now();

    print_measurement(container_name, sequence_name, "insert", data.size(), unique_count,
                      data.size(), finish_insert - start_insert);
    print_measurement(container_name, sequence_name, "select", data.size(), unique_count,
                      data.size(), finish_select - start_select, select_checksum);
    print_measurement(container_name, sequence_name, "count_less", data.size(), unique_count,
                      data.size(), finish_count - start_count, count_checksum);
    print_measurement(container_name, sequence_name,
                      unique_count == 1 ? "range_count узкий (пустой диапазон, ранний выход)" : "range_count узкий",
                      data.size(), unique_count, data.size(), finish_narrow - start_narrow, narrow_checksum);
    print_measurement(container_name, sequence_name,
                      unique_count == 1 ? "range_count широкий (пустой диапазон, ранний выход)" : "range_count широкий",
                      data.size(), unique_count, data.size(), finish_wide - start_wide, wide_checksum);
}

void benchmark_sequence(const char* sequence_name, std::vector<int> data)
{
    if (data.empty())
    {
        std::cout << sequence_name << " | пустой data, запросы пропущены\n";
        return;
    }

    std::vector<int> ordered = data;
    std::sort(ordered.begin(), ordered.end());
    ordered.erase(std::unique(ordered.begin(), ordered.end()), ordered.end());

    std::vector<std::size_t> ranks;
    std::vector<std::pair<int, int>> narrow_ranges;
    std::vector<std::pair<int, int>> wide_ranges;
    ranks.reserve(data.size());
    narrow_ranges.reserve(data.size());
    wide_ranges.reserve(data.size());

    for (int key : data)
    {
        const std::size_t position = std::lower_bound(ordered.begin(), ordered.end(), key) - ordered.begin();
        ranks.push_back(position + 1);
        if (ordered.size() == 1)
        {
            narrow_ranges.emplace_back(key, key);
            wide_ranges.emplace_back(key, key);
        }
        else
        {
            const std::size_t narrow_left = std::min(position, ordered.size() - 2);
            const std::size_t width = ordered.size() / 2;
            const std::size_t wide_left = std::min(position, ordered.size() - width - 1);
            narrow_ranges.emplace_back(ordered[narrow_left], ordered[narrow_left + 1]);
            wide_ranges.emplace_back(ordered[wide_left], ordered[wide_left + width]);
        }
    }

    {
        search_tools::B_Tree<int> b_tree(2);
        benchmark_container(b_tree, "B_Tree<int>", sequence_name, data, ordered, ranks, narrow_ranges, wide_ranges);
    }

    {
        search_tools::Set_Adapter<int> set_adapter;
        benchmark_container(set_adapter, "Set_Adapter<int>", sequence_name, data, ordered, ranks, narrow_ranges, wide_ranges);
    }

    {
        search_tools::Sorted_Vector<int> sorted_vector;
        benchmark_container(sorted_vector, "Sorted_Vector<int>", sequence_name, data, ordered, ranks, narrow_ranges, wide_ranges);
    }

    {
        search_tools::AVL_Tree<int> avl_tree;
        benchmark_container(avl_tree, "AVL_Tree<int>", sequence_name, data, ordered, ranks, narrow_ranges, wide_ranges);
    }
}

template <typename Container>
void benchmark_select_cases(const Container& container, const char* container_name,
                            const std::vector<int>& data)
{
    using Clock = std::chrono::steady_clock;
    const std::size_t unique_count = container.size();
    const std::size_t repetitions = data.size() / 3;

    {
        const std::size_t index = 1;
        std::uint64_t checksum = 0;
        const auto start = Clock::now();
        for (std::size_t i = 0; i < repetitions; ++i)
        {
#if defined(__GNUC__) || defined(__clang__)
            asm volatile("" : : "g"(&container) : "memory");
#endif
            checksum += static_cast<std::uint64_t>(container.select(index).value_or(0));
        }
        const auto finish = Clock::now();
        print_measurement(container_name, "Strictly_increasing", "select", data.size(),
                          unique_count, repetitions, finish - start, checksum, "First");
    }

    {
        const std::size_t index = 1 + unique_count / 2;
        std::uint64_t checksum = 0;
        const auto start = Clock::now();
        for (std::size_t i = 0; i < repetitions; ++i)
        {
#if defined(__GNUC__) || defined(__clang__)
            asm volatile("" : : "g"(&container) : "memory");
#endif
            checksum += static_cast<std::uint64_t>(container.select(index).value_or(0));
        }
        const auto finish = Clock::now();
        print_measurement(container_name, "Strictly_increasing", "select", data.size(),
                          unique_count, repetitions, finish - start, checksum, "Middle");
    }

    {
        const std::size_t index = unique_count;
        std::uint64_t checksum = 0;
        const auto start = Clock::now();
        for (std::size_t i = 0; i < repetitions; ++i)
        {
#if defined(__GNUC__) || defined(__clang__)
            asm volatile("" : : "g"(&container) : "memory");
#endif
            checksum += static_cast<std::uint64_t>(container.select(index).value_or(0));
        }
        const auto finish = Clock::now();
        print_measurement(container_name, "Strictly_increasing", "select", data.size(),
                          unique_count, repetitions, finish - start, checksum, "Last");
    }
}

template <typename Container>
void benchmark_count_less_cases(const Container& container, const char* container_name,
                                const std::vector<int>& data)
{
    using Clock = std::chrono::steady_clock;
    const std::size_t unique_count = container.size();
    const std::size_t repetitions = data.size() / 3;

    {
        const int key = data.front();
        std::uint64_t checksum = 0;
        const auto start = Clock::now();
        for (std::size_t i = 0; i < repetitions; ++i)
        {
#if defined(__GNUC__) || defined(__clang__)
            asm volatile("" : : "g"(&container) : "memory");
#endif
            checksum += container.count_less(key);
        }
        const auto finish = Clock::now();
        print_measurement(container_name, "Strictly_increasing", "count_less", data.size(),
                          unique_count, repetitions, finish - start, checksum, "First");
    }

    {
        const int key = data[unique_count / 2];
        std::uint64_t checksum = 0;
        const auto start = Clock::now();
        for (std::size_t i = 0; i < repetitions; ++i)
        {
#if defined(__GNUC__) || defined(__clang__)
            asm volatile("" : : "g"(&container) : "memory");
#endif
            checksum += container.count_less(key);
        }
        const auto finish = Clock::now();
        print_measurement(container_name, "Strictly_increasing", "count_less", data.size(),
                          unique_count, repetitions, finish - start, checksum, "Middle");
    }

    {
        const int key = data.back();
        std::uint64_t checksum = 0;
        const auto start = Clock::now();
        for (std::size_t i = 0; i < repetitions; ++i)
        {
#if defined(__GNUC__) || defined(__clang__)
            asm volatile("" : : "g"(&container) : "memory");
#endif
            checksum += container.count_less(key);
        }
        const auto finish = Clock::now();
        print_measurement(container_name, "Strictly_increasing", "count_less", data.size(),
                          unique_count, repetitions, finish - start, checksum, "Last");
    }
}

void benchmark_strictly_increasing_cases(std::size_t size_data)
{
    std::cout << "=== Дополнительные замеры Strictly_increasing ===\n";
    const std::vector<int> data = Strictly_increasing(size_data);
    if (data.size() / 3 == 0)
    {
        std::cout << "Дополнительный эксперимент пропущен: N / 3 == 0, нужно N >= 3\n";
        return;
    }

    {
        search_tools::B_Tree<int> container(2);
        for (int key : data)
        {
            container.insert(key);
        }
        benchmark_select_cases(container, "B_Tree<int>", data);
        benchmark_count_less_cases(container, "B_Tree<int>", data);
    }

    {
        search_tools::Set_Adapter<int> container;
        for (int key : data)
        {
            container.insert(key);
        }
        benchmark_select_cases(container, "Set_Adapter<int>", data);
        benchmark_count_less_cases(container, "Set_Adapter<int>", data);
    }

    {
        search_tools::Sorted_Vector<int> container;
        for (int key : data)
        {
            container.insert(key);
        }
        benchmark_select_cases(container, "Sorted_Vector<int>", data);
        benchmark_count_less_cases(container, "Sorted_Vector<int>", data);
    }

    {
        search_tools::AVL_Tree<int> container;
        for (int key : data)
        {
            container.insert(key);
        }
        benchmark_select_cases(container, "AVL_Tree<int>", data);
        benchmark_count_less_cases(container, "AVL_Tree<int>", data);
    }
}

} //namespace

std::vector<int> Strictly_increasing(std::size_t size_data)
{
    std::vector<int> data;
    data.reserve(size_data);

    for (std::size_t i = 0; i < size_data; ++i)
    {
        data.push_back(static_cast<int>(i + 1));
    }

    return data;
}

std::vector<int> Strictly_decreasing(std::size_t size_data)
{
    std::vector<int> data;
    data.reserve(size_data);

    for (std::size_t i = 0; i < size_data; ++i)
    {
        data.push_back(static_cast<int>(size_data - i));
    }

    return data;
}

std::vector<int> Random(std::size_t size_data)
{
    std::vector<int> data = Strictly_increasing(size_data);
    std::mt19937 generator(42);
    std::shuffle(data.begin(), data.end(), generator);

    return data;
}

std::vector<int> All_equal(std::size_t size_data)
{
    return std::vector<int>(size_data, 1);
}

std::vector<int> With_duplicates(std::size_t size_data)
{
    std::vector<int> data;
    data.reserve(size_data);

    for (std::size_t i = 0; i < size_data; ++i)
    {
        data.push_back(static_cast<int>(i / 3 + 1));
    }

    return data;
}

std::vector<int> Alternating_low_high(std::size_t size_data)
{
    std::vector<int> data;
    data.reserve(size_data);

    for (std::size_t i = 0; i < size_data; ++i)
    {
        const std::size_t key = (i % 2 == 0) ? i / 2 + 1 : size_data - i / 2;
        data.push_back(static_cast<int>(key));
    }

    return data;
}

std::vector<int> Nearly_sorted(std::size_t size_data)
{
    std::vector<int> data = Strictly_increasing(size_data);

    if (size_data > 1)
    {
        std::swap(data[size_data / 2 - 1], data[size_data / 2]);
    }

    return data;
}

std::vector<int> With_negative_numbers_and_zero(std::size_t size_data)
{
    std::vector<int> data;
    data.reserve(size_data);

    for (std::size_t i = 0; i < size_data; ++i)
    {
        data.push_back(static_cast<int>(i) - static_cast<int>(size_data / 2));
    }

    return data;
}

std::vector<int> With_extreme_values(std::size_t size_data)
{
    std::vector<int> data;
    data.reserve(size_data);

    for (std::size_t i = 0; i < size_data; ++i)
    {
        switch (i % 5)
        {
            case 0: data.push_back(std::numeric_limits<int>::min()); break;
            case 1: data.push_back(std::numeric_limits<int>::max()); break;
            case 2: data.push_back(0); break;
            case 3: data.push_back(-1); break;
            case 4: data.push_back(1); break;
        }
    }

    return data;
}

std::vector<int> Single_element(std::size_t size_data)
{
    return {static_cast<int>(size_data)};
}

void do_benchmark_test()
{
    std::cout << "Введи число эллементов: ";

    std::string input;
    std::cin >> input;

    std::cout << "\n";

    std::size_t size_data = 0;
    const auto result = std::from_chars(input.data(), input.data() + input.size(), size_data);

    if (!std::cin || result.ec != std::errc{} || result.ptr != input.data() + input.size() || size_data < 1)
    {
        throw std::invalid_argument("N должно быть целым числом от 1 до " + std::to_string(std::numeric_limits<int>::max()));
    }

    std::cout << "=== Исследование паттернов ===\n";
    
    benchmark_sequence("Strictly_increasing", Strictly_increasing(size_data));
    benchmark_sequence("Strictly_decreasing", Strictly_decreasing(size_data));
    benchmark_sequence("Random", Random(size_data));
    benchmark_sequence("All_equal", All_equal(size_data));
    benchmark_sequence("With_duplicates", With_duplicates(size_data));
    benchmark_sequence("Alternating_low_high", Alternating_low_high(size_data));
    benchmark_sequence("Nearly_sorted", Nearly_sorted(size_data));
    benchmark_sequence("With_negative_numbers_and_zero", With_negative_numbers_and_zero(size_data));
    benchmark_sequence("With_extreme_values", With_extreme_values(size_data));
    benchmark_sequence("Single_element", Single_element(size_data));

    benchmark_strictly_increasing_cases(size_data);
}
