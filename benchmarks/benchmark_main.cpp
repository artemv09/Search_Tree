#include <iostream>
#include <cstdlib>
#include <exception>

#include "benchmark.hpp"

int main()
{
    try
    {
        do_benchmark_test();
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Ошибка бенчмарка: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
