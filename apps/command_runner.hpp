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
int run_commands(Container& container, std::istream& commands,
                 std::ostream& results, std::ostream& errors)
{
    for (std::size_t command_number = 1; ; ++command_number)
    {
        try
        {
            input::Parsed_Command command;
            const auto status = input::read_command(commands, command, errors, command_number);

            if (status == input::Read_Status::EndOfInput)
            {
                if (!results)
                {
                    throw std::runtime_error("ошибка вывода результатов");
                }
                return EXIT_SUCCESS;
            }

            if (status == input::Read_Status::Error)
            {
                return EXIT_FAILURE;
            }

            if (!container.execution_command(command, results))
            {
                throw std::runtime_error("не удалось выполнить команду");
            }

            if (command.type != input::Command::Insert)
            {
                results << '\n';
            }
        }

        catch (const std::exception& error)
        {
            errors << "Команда " << command_number << ": " << error.what() << '\n';
            return EXIT_FAILURE;
        }
    }
}

template <typename Container>
int run_commands(Container& container)
{
    return run_commands(container, std::cin, std::cout, std::cerr);
}

} //namespace search_tree_apps

#endif
