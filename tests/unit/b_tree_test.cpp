#include <gtest/gtest.h>
#include <functional>
#include <initializer_list>
#include <optional>
#include <stdexcept>

#include "b_tree.hpp"

TEST(BTreeTest, Empty_Select)
{
    search_tools::B_Tree<int> container(2);

    EXPECT_EQ(container.select(0), std::nullopt);
    EXPECT_EQ(container.select(1), std::nullopt);
    EXPECT_EQ(container.count_less(2), 0u);
    EXPECT_EQ(container.range_count(-1, 8), 0u);
    EXPECT_EQ(container.range_count(2, 2), 0u);
}

TEST(BTreeTest, Insert_One)
{
    search_tools::B_Tree<int> container(2);
    container.insert(2);

    EXPECT_EQ(container.select(1), std::optional<int>{2});
    EXPECT_EQ(container.select(0), std::nullopt);
    EXPECT_EQ(container.select(2), std::nullopt);
}

TEST(BTreeTest, Unsorted_Insert)
{
    search_tools::B_Tree<int> container(2);
    for (int key : {8, -1, 2})
    {
        container.insert(key);
    }

    EXPECT_EQ(container.select(1), std::optional<int>{-1});
    EXPECT_EQ(container.select(2), std::optional<int>{2});
    EXPECT_EQ(container.select(3), std::optional<int>{8});
    EXPECT_EQ(container.select(4), std::nullopt);
}

TEST(BTreeTest, No_Duplicates)
{
    search_tools::B_Tree<int> container(2);
    container.insert(2);
    container.insert(2);

    EXPECT_EQ(container.select(1), std::optional<int>{2});
    EXPECT_EQ(container.select(2), std::nullopt);
    EXPECT_EQ(container.count_less(3), 1u);
}

TEST(BTreeTest, Count_Less)
{
    search_tools::B_Tree<int> container(2);
    for (int key : {8, -1, 2})
    {
        container.insert(key);
    }

    EXPECT_EQ(container.count_less(-2), 0u);
    EXPECT_EQ(container.count_less(-1), 0u);
    EXPECT_EQ(container.count_less(0), 1u);
    EXPECT_EQ(container.count_less(2), 1u);
    EXPECT_EQ(container.count_less(3), 2u);
    EXPECT_EQ(container.count_less(8), 2u);
    EXPECT_EQ(container.count_less(9), 3u);
}

TEST(BTreeTest, Range_Bounds)
{
    search_tools::B_Tree<int> container(2);
    for (int key : {8, -1, 2})
    {
        container.insert(key);
    }

    EXPECT_EQ(container.range_count(-1, 8), 2u);
    EXPECT_EQ(container.range_count(-1, 2), 1u);
    EXPECT_EQ(container.range_count(2, 8), 1u);
    EXPECT_EQ(container.range_count(-2, 9), 3u);
    EXPECT_EQ(container.range_count(0, 7), 1u);
    EXPECT_EQ(container.range_count(3, 7), 0u);
    EXPECT_EQ(container.range_count(2, 2), 0u);
    EXPECT_EQ(container.range_count(0, 0), 0u);
}

TEST(BTreeTest, Reverse_Range)
{
    search_tools::B_Tree<int> container(2);
    EXPECT_THROW((void)container.range_count(10, 5), std::invalid_argument);

    container.insert(2);
    EXPECT_THROW((void)container.range_count(10, 5), std::invalid_argument);
}

TEST(BTreeTest, Descending_Order)
{
    search_tools::B_Tree<int, std::greater<int>> container(2);
    for (int key : {2, -1, 8})
    {
        container.insert(key);
    }

    EXPECT_EQ(container.select(1), std::optional<int>{8});
    EXPECT_EQ(container.select(2), std::optional<int>{2});
    EXPECT_EQ(container.select(3), std::optional<int>{-1});
    EXPECT_EQ(container.select(4), std::nullopt);
    // В порядке greater раньше x находятся ключи, численно большие x.
    EXPECT_EQ(container.count_less(9), 0u);
    EXPECT_EQ(container.count_less(8), 0u);
    EXPECT_EQ(container.count_less(2), 1u);
    EXPECT_EQ(container.count_less(0), 2u);
    EXPECT_EQ(container.count_less(-2), 3u);
    EXPECT_EQ(container.range_count(8, -1), 2u);
    EXPECT_EQ(container.range_count(8, 2), 1u);
    EXPECT_EQ(container.range_count(2, 2), 0u);
    EXPECT_THROW((void)container.range_count(-1, 8), std::invalid_argument);
}

TEST(BTreeTest, Node_Splits)
{
    search_tools::B_Tree<int> container(2);

    // При t = 2 эта последовательность вызывает разделения корня и детей.
    for (int key : {1, 2, 3, 4, 5, 6, 7, 8})
    {
        container.insert(key);
    }
    container.insert(4);

    EXPECT_EQ(container.select(1), std::optional<int>{1});
    EXPECT_EQ(container.select(2), std::optional<int>{2});
    EXPECT_EQ(container.select(3), std::optional<int>{3});
    EXPECT_EQ(container.select(4), std::optional<int>{4});
    EXPECT_EQ(container.select(5), std::optional<int>{5});
    EXPECT_EQ(container.select(6), std::optional<int>{6});
    EXPECT_EQ(container.select(7), std::optional<int>{7});
    EXPECT_EQ(container.select(8), std::optional<int>{8});
    EXPECT_EQ(container.select(9), std::nullopt);
    
    EXPECT_EQ(container.count_less(1), 0u);
    EXPECT_EQ(container.count_less(4), 3u);
    EXPECT_EQ(container.count_less(9), 8u);
    EXPECT_EQ(container.range_count(2, 6), 4u);
}
