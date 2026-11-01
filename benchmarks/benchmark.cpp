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

#include "b_tree.hpp"
#include "set_adapter.hpp"
#include "sorted_vector.hpp"
#include "threaded_avl_tree.hpp"

namespace {

void print_measurement(const char* container_name, const char* sequence_name,
                       const char* method, std::size_t insertions,
                       std::size_t unique_count, std::size_t operations,
                       std::optional<std::chrono::steady_clock::duration> elapsed,
                       std::optional<std::uint64_t> checksum = std::nullopt)
{
    std::cout << container_name << " | " << sequence_name
              << " | вставок = " << insertions << " | уникальных = " << unique_count
              << " | метод = " << method << " | операций = " << operations;

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
                         const std::vector<int>& ordered)
{
    using Clock = std::chrono::steady_clock;
    const std::size_t unique_count = ordered.size();
    const std::size_t narrow_operations = unique_count >= 2 ? unique_count - 1 : 0;
    const std::size_t width = unique_count / 2;
    const std::size_t wide_operations = unique_count >= 2 ? unique_count - width : 0;

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

    for (std::size_t i = 1; i <= unique_count; ++i)
    {
        const auto value = container.select(i);
        select_checksum += value.value_or(0);
    }

    const auto finish_select = Clock::now();

    std::uint64_t count_checksum = 0;
    const auto start_count = Clock::now();

    for (int key : ordered)
    {
        count_checksum += container.count_less(key);
    }

    const auto finish_count = Clock::now();

    std::uint64_t narrow_checksum = 0;
    std::optional<Clock::duration> narrow_elapsed;
    std::uint64_t wide_checksum = 0;
    std::optional<Clock::duration> wide_elapsed;

    if (unique_count >= 2)
    {
        const auto start_narrow = Clock::now();

        for (std::size_t i = 1; i < unique_count; ++i)
        {
            narrow_checksum += container.range_count(ordered[i - 1], ordered[i]);
        }

        const auto finish_narrow = Clock::now();
        narrow_elapsed = finish_narrow - start_narrow;

        const auto start_wide = Clock::now();

        for (std::size_t i = 0; i < unique_count - width; ++i)
        {
            wide_checksum += container.range_count(ordered[i], ordered[i + width]);
        }

        const auto finish_wide = Clock::now();
        wide_elapsed = finish_wide - start_wide;
    }

    print_measurement(container_name, sequence_name, "insert", data.size(), unique_count,
                      data.size(), finish_insert - start_insert);
    print_measurement(container_name, sequence_name, "select", data.size(), unique_count,
                      unique_count, finish_select - start_select, select_checksum);
    print_measurement(container_name, sequence_name, "count_less", data.size(), unique_count,
                      unique_count, finish_count - start_count, count_checksum);
    print_measurement(container_name, sequence_name, "range_count узкий", data.size(), unique_count,
                      narrow_operations, narrow_elapsed, narrow_checksum);
    print_measurement(container_name, sequence_name, "range_count широкий", data.size(), unique_count,
                      wide_operations, wide_elapsed, wide_checksum);
}

void benchmark_sequence(const char* sequence_name, std::vector<int> data)
{
    std::vector<int> ordered = data;
    std::sort(ordered.begin(), ordered.end());
    ordered.erase(std::unique(ordered.begin(), ordered.end()), ordered.end());

    {
        search_tools::B_Tree<int> b_tree(2);
        benchmark_container(b_tree, "B_Tree<int>", sequence_name, data, ordered);
    }

    {
        search_tools::Set_Adapter<int> set_adapter;
        benchmark_container(set_adapter, "Set_Adapter<int>", sequence_name, data, ordered);
    }

    {
        search_tools::Sorted_Vector<int> sorted_vector;
        benchmark_container(sorted_vector, "Sorted_Vector<int>", sequence_name, data, ordered);
    }

    {
        search_tools::AVL_Tree<int> avl_tree;
        benchmark_container(avl_tree, "AVL_Tree<int>", sequence_name, data, ordered);
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


}
