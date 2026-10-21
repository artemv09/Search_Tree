#include "input.hpp"

#include <charconv>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <system_error>

namespace input {

namespace detail {

bool read_token(std::istream& stream, std::string& token)
{
    token.clear();

    try 
    {
        stream >> std::ws >> token;
    } 
    catch (const std::ios_base::failure&) 
    {
        if (!stream.bad() && stream.eof()) 
        {
            if (token.empty()) 
            {
                return false;
            }
            if (!stream.fail()) 
            {
                return true;
            }
        }
        throw;
    }

    if (stream.bad()) 
    {
        throw std::runtime_error("ошибка устройства ввода");
    }
    if (stream.fail()) 
    {
        if (stream.eof() && token.empty()) 
        {
            return false;
        }
        throw std::runtime_error("ошибка чтения входного потока");
    }
    return true;
}

void read_argument(std::istream& stream, std::string& token, const char* name)
{
    if (!read_token(stream, token)) 
    {
        throw std::runtime_error(std::string("незавершённая команда: отсутствует аргумент ") + name);
    }
}

template <typename Integer>
Integer parse_integer(std::string_view token, const char* name) //преобразовать строку в число
{
    if (!token.empty() && token.front() == '+') 
    {
        token.remove_prefix(1);
        if (!token.empty() && (token.front() == '+' || token.front() == '-')) //если два минуса или два плюса
        {
            throw std::runtime_error(std::string("аргумент ") + name + ": неверная запись целого числа");
        }
    }
    if (token.empty()) 
    {
        throw std::runtime_error(std::string("аргумент ") + name + ": ожидалось целое число");
    }

    Integer value = 0;

    const char* const end = token.data() + token.size();
    const auto result = std::from_chars(token.data(), end, value, 10);

    if (result.ptr != end || result.ec == std::errc::invalid_argument)
    {
        throw std::runtime_error(std::string("аргумент ") + name + ": неверная запись целого числа");
    }
    if (result.ec == std::errc::result_out_of_range) 
    {
        throw std::runtime_error(std::string("аргумент ") + name + ": число вне диапазона целевого типа");
    }

    return value;
}

void report_error(std::ostream& errors, std::size_t command_number, const char* reason) noexcept
{
    try 
    {
        errors << "Команда " << command_number << ": " << reason << '\n';
    } 
    catch (...) 
    {
    }
}

} //namespace detail

[[nodiscard]] Read_Status read_command(std::istream& stream,
                                       Parsed_Command& output,
                                       std::ostream& errors,
                                       std::size_t command_number
                                       ) noexcept //возвращаем output
{
    try 
    {
        std::string token;
        if (!detail::read_token(stream, token)) 
        {
            return Read_Status::EndOfInput;
        }

        Parsed_Command parsed;

        if (token == "k") 
        {
            parsed.type = Command::Insert;
        } 
        else if (token == "q") 
        {
            parsed.type = Command::RangeCount;
        } 
        else if (token == "m") 
        {
            parsed.type = Command::Select;
        } 
        else if (token == "n") 
        {
            parsed.type = Command::CountLess;
        } 
        else 
        {
            throw std::runtime_error("неизвестная команда: " + token);
        }

        switch (parsed.type) 
        {
            case Command::Insert:
            case Command::CountLess:
            {
                detail::read_argument(stream, token, "x");
                parsed.argument = detail::parse_integer<int>(token, "x");
                break;
            }
            case Command::RangeCount:
            {
                detail::read_argument(stream, token, "l");
                parsed.argument = detail::parse_integer<int>(token, "l");
                detail::read_argument(stream, token, "r");
                parsed.right_bound = detail::parse_integer<int>(token, "r");
                break;
            }
            case Command::Select:
            {
                detail::read_argument(stream, token, "i");
                if (!token.empty() && token.front() == '-') 
                {
                    throw std::runtime_error("аргумент i: номер должен быть положительным");
                }

                parsed.index_one_based = detail::parse_integer<std::size_t>(token, "i");
                
                if (parsed.index_one_based == 0) 
                {
                    throw std::runtime_error("аргумент i: номер должен быть положительным");
                }
                break;
            }
        }

        output = parsed;
        return Read_Status::CommandRead;
    } 
    catch (const std::exception& error) 
    {
        detail::report_error(errors, command_number, error.what());
    } 
    catch (...) 
    {
        detail::report_error(errors, command_number, "неизвестное исключение при чтении команды");
    }
    
    return Read_Status::Error;
}

} // namespace input
