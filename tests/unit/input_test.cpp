#include <gtest/gtest.h>
#include <limits>
#include <sstream>
#include <string>

#include "input.hpp"

TEST(InputTest, Parse_Insert)
{
    std::istringstream commands("k -1\n");
    std::ostringstream errors;
    input::Parsed_Command command;

    ASSERT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::CommandRead);
    EXPECT_EQ(command.type, input::Command::Insert);
    EXPECT_EQ(command.argument, -1);
    EXPECT_TRUE(errors.str().empty());
}

TEST(InputTest, Parse_Range)
{
    std::istringstream commands("q -1 8\n");
    std::ostringstream errors;
    input::Parsed_Command command;

    ASSERT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::CommandRead);
    EXPECT_EQ(command.type, input::Command::RangeCount);
    EXPECT_EQ(command.argument, -1);
    EXPECT_EQ(command.right_bound, 8);
    EXPECT_TRUE(errors.str().empty());
}

TEST(InputTest, Parse_Select)
{
    std::istringstream commands("m 2\n");
    std::ostringstream errors;
    input::Parsed_Command command;

    ASSERT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::CommandRead);
    EXPECT_EQ(command.type, input::Command::Select);
    EXPECT_EQ(command.index_one_based, 2u);
    EXPECT_TRUE(errors.str().empty());
}

TEST(InputTest, Parse_Count_Less)
{
    std::istringstream commands("n 3\n");
    std::ostringstream errors;
    input::Parsed_Command command;

    ASSERT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::CommandRead);
    EXPECT_EQ(command.type, input::Command::CountLess);
    EXPECT_EQ(command.argument, 3);
    EXPECT_TRUE(errors.str().empty());
}

TEST(InputTest, Read_Several_Commands)
{
    std::istringstream commands("k 2 n 3\n");
    std::ostringstream errors;
    input::Parsed_Command command;

    ASSERT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::CommandRead);
    EXPECT_EQ(command.type, input::Command::Insert);
    EXPECT_EQ(command.argument, 2);

    ASSERT_EQ(input::read_command(commands, command, errors, 2), input::Read_Status::CommandRead);
    EXPECT_EQ(command.type, input::Command::CountLess);
    EXPECT_EQ(command.argument, 3);

    EXPECT_EQ(input::read_command(commands, command, errors, 3), input::Read_Status::EndOfInput);
    EXPECT_TRUE(errors.str().empty());
}

TEST(InputTest, Empty_Input)
{
    std::istringstream commands("");
    std::ostringstream errors;
    input::Parsed_Command command;

    EXPECT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::EndOfInput);
    EXPECT_TRUE(errors.str().empty());
}

TEST(InputTest, No_Final_Newline)
{
    std::istringstream commands("k 8");
    std::ostringstream errors;
    input::Parsed_Command command;

    ASSERT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::CommandRead);
    EXPECT_EQ(command.type, input::Command::Insert);
    EXPECT_EQ(command.argument, 8);
    EXPECT_EQ(input::read_command(commands, command, errors, 2), input::Read_Status::EndOfInput);
    EXPECT_TRUE(errors.str().empty());
}

TEST(InputTest, Unknown_Command)
{
    std::istringstream commands("z 2");
    std::ostringstream errors;
    input::Parsed_Command command;

    EXPECT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::Error);
    EXPECT_FALSE(errors.str().empty());
}

TEST(InputTest, Missing_Argument)
{
    for (const char* text : {"k", "q 1", "m", "n"})
    {
        std::istringstream commands(text);
        std::ostringstream errors;
        input::Parsed_Command command;

        EXPECT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::Error);
        EXPECT_FALSE(errors.str().empty());
    }
}

TEST(InputTest, Invalid_Number)
{
    std::istringstream commands("k abc");
    std::ostringstream errors;
    input::Parsed_Command command;

    EXPECT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::Error);
    EXPECT_FALSE(errors.str().empty());
}

TEST(InputTest, Integer_Overflow)
{
    // Приписанный ноль делает число больше максимального int.
    std::istringstream commands("k " + std::to_string(std::numeric_limits<int>::max()) + "0");
    std::ostringstream errors;
    input::Parsed_Command command;

    EXPECT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::Error);
    EXPECT_FALSE(errors.str().empty());
}

TEST(InputTest, Invalid_Index)
{
    for (const char* text : {"m 0", "m -1"})
    {
        std::istringstream commands(text);
        std::ostringstream errors;
        input::Parsed_Command command;

        EXPECT_EQ(input::read_command(commands, command, errors, 1), input::Read_Status::Error);
        EXPECT_FALSE(errors.str().empty());
    }
}
