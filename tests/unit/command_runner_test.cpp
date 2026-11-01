#include <gtest/gtest.h>
#include <cstdlib>
#include <optional>
#include <sstream>

#include "../../apps/command_runner.hpp"
#include "sorted_vector.hpp"

TEST(CommandRunnerTest, Empty_Input)
{
    search_tools::Sorted_Vector<int> container;
    std::istringstream commands("");
    std::ostringstream results;
    std::ostringstream errors;

    EXPECT_EQ(search_tree_apps::run_commands(container, commands, results, errors), EXIT_SUCCESS);
    EXPECT_TRUE(results.str().empty());
    EXPECT_TRUE(errors.str().empty());
    EXPECT_EQ(container.select(1), std::nullopt);
}

TEST(CommandRunnerTest, Task_Example)
{
    search_tools::Sorted_Vector<int> container;
    
    std::istringstream commands("k 8 k 2 k -1 m 1 m 2 n 3 q -1 8");
    std::ostringstream results;
    std::ostringstream errors;

    EXPECT_EQ(search_tree_apps::run_commands(container, commands, results, errors), EXIT_SUCCESS);
    EXPECT_EQ(results.str(), "-1\n2\n2\n2\n");
    EXPECT_TRUE(errors.str().empty());
}

TEST(CommandRunnerTest, Insert_Without_Output)
{
    search_tools::Sorted_Vector<int> container;
    std::istringstream commands("k 8 k 2 k -1");
    std::ostringstream results;
    std::ostringstream errors;

    EXPECT_EQ(search_tree_apps::run_commands(container, commands, results, errors), EXIT_SUCCESS);
    EXPECT_TRUE(results.str().empty());
    EXPECT_TRUE(errors.str().empty());
    EXPECT_EQ(container.select(1), std::optional<int>{-1});
    EXPECT_EQ(container.select(2), std::optional<int>{2});
    EXPECT_EQ(container.select(3), std::optional<int>{8});
    EXPECT_EQ(container.select(4), std::nullopt);
}

TEST(CommandRunnerTest, Stop_On_Error)
{
    search_tools::Sorted_Vector<int> container;
    std::istringstream commands("k 2 z k 8");
    std::ostringstream results;
    std::ostringstream errors;

    EXPECT_EQ(search_tree_apps::run_commands(container, commands, results, errors), EXIT_FAILURE);
    EXPECT_FALSE(errors.str().empty());
    EXPECT_TRUE(results.str().empty());
    EXPECT_EQ(container.select(1), std::optional<int>{2});
    EXPECT_EQ(container.select(2), std::nullopt);
}

TEST(CommandRunnerTest, Invalid_Select)
{
    search_tools::Sorted_Vector<int> container;
    std::istringstream commands("k 2 m 2 k 8");
    std::ostringstream results;
    std::ostringstream errors;

    EXPECT_EQ(search_tree_apps::run_commands(container, commands, results, errors), EXIT_FAILURE);
    EXPECT_FALSE(errors.str().empty());
    EXPECT_TRUE(results.str().empty());
    EXPECT_EQ(container.select(1), std::optional<int>{2});
    EXPECT_EQ(container.select(2), std::nullopt);
}

TEST(CommandRunnerTest, Reverse_Range)
{
    search_tools::Sorted_Vector<int> container;
    std::istringstream commands("k 2 q 10 5 k 8");
    std::ostringstream results;
    std::ostringstream errors;

    EXPECT_EQ(search_tree_apps::run_commands(container, commands, results, errors), EXIT_FAILURE);
    EXPECT_FALSE(errors.str().empty());
    EXPECT_TRUE(results.str().empty());
    EXPECT_EQ(container.select(1), std::optional<int>{2});
    EXPECT_EQ(container.select(2), std::nullopt);
}
