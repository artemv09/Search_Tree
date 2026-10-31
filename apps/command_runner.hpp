#ifndef SEARCH_TREE_COMMAND_RUNNER_HPP
#define SEARCH_TREE_COMMAND_RUNNER_HPP

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "input.hpp"

namespace search_tree_apps {

template <typename Container>
int run_commands(Container& container)
{
    for (std::size_t command_number = 1; ; ++command_number)
    {
        try
        {
            input::Parsed_Command command;
            const auto status = input::read_command(std::cin, command, std::cerr, command_number);

            if (status == input::Read_Status::EndOfInput)
            {
                if (!std::cout)
                {
                    throw std::runtime_error("ошибка вывода результатов");
                }
                return EXIT_SUCCESS;
            }

            if (status == input::Read_Status::Error)
            {
                return EXIT_FAILURE;
            }

            if (!container.execution_command(command, std::cout))
            {
                throw std::runtime_error("не удалось выполнить команду");
            }

            if (command.type != input::Command::Insert)
            {
                std::cout << '\n';
            }
        }

        catch (const std::exception& error)
        {
            std::cerr << "Команда " << command_number << ": " << error.what() << '\n';
            return EXIT_FAILURE;
        }
    }
}

} //namespace search_tree_apps

#endif
