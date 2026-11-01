#ifndef SEARCH_TREE_BENCHMARK_
#define SEARCH_TREE_BENCHMARK_

#include <cstddef>
#include <vector>

std::vector<int> Strictly_increasing(std::size_t size_data);
std::vector<int> Strictly_decreasing(std::size_t size_data);
std::vector<int> Random(std::size_t size_data);
std::vector<int> All_equal(std::size_t size_data);
std::vector<int> With_duplicates(std::size_t size_data);
std::vector<int> Alternating_low_high(std::size_t size_data);
std::vector<int> Nearly_sorted(std::size_t size_data);
std::vector<int> With_negative_numbers_and_zero(std::size_t size_data);
std::vector<int> With_extreme_values(std::size_t size_data);
std::vector<int> Single_element(std::size_t size_data);

void do_benchmark_test();

#endif
