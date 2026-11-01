#include <gtest/gtest.h>
#include <functional>
#include <optional>
#include <stdexcept>

#include "threaded_avl_tree.hpp"

TEST(AVLTreeTest, Empty_Select)
{
    search_tools::AVL_Tree<int> container;

    EXPECT_EQ(container.select(0), std::nullopt);
    EXPECT_EQ(container.select(1), std::nullopt);
    EXPECT_EQ(container.count_less(2), 0u);
    EXPECT_EQ(container.range_count(-1, 8), 0u);
    EXPECT_EQ(container.range_count(2, 2), 0u);
}

TEST(AVLTreeTest, Insert_One)
{
    search_tools::AVL_Tree<int> container;
    container.insert(2);

    EXPECT_EQ(container.select(1), std::optional<int>{2});
    EXPECT_EQ(container.select(0), std::nullopt);
    EXPECT_EQ(container.select(2), std::nullopt);
}

TEST(AVLTreeTest, Unsorted_Insert)
{
    search_tools::AVL_Tree<int> container;
    for (int key : {8, -1, 2})
    {
        container.insert(key);
    }

    EXPECT_EQ(container.select(1), std::optional<int>{-1});
    EXPECT_EQ(container.select(2), std::optional<int>{2});
    EXPECT_EQ(container.select(3), std::optional<int>{8});
    EXPECT_EQ(container.select(4), std::nullopt);
}

TEST(AVLTreeTest, No_Duplicates)
{
    search_tools::AVL_Tree<int> container;
    container.insert(2);
    container.insert(2);

    EXPECT_EQ(container.select(1), std::optional<int>{2});
    EXPECT_EQ(container.select(2), std::nullopt);
    EXPECT_EQ(container.count_less(3), 1u);
}

TEST(AVLTreeTest, Count_Less)
{
    search_tools::AVL_Tree<int> container;
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

TEST(AVLTreeTest, Range_Bounds)
{
    search_tools::AVL_Tree<int> container;
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

TEST(AVLTreeTest, Reverse_Range)
{
    search_tools::AVL_Tree<int> container;
    EXPECT_THROW((void)container.range_count(10, 5), std::invalid_argument);

    container.insert(2);
    EXPECT_THROW((void)container.range_count(10, 5), std::invalid_argument);
}

TEST(AVLTreeTest, Descending_Order)
{
    search_tools::AVL_Tree<int, std::greater<int>> container;
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

TEST(AVLTreeTest, Single_Rotations)
{
    // LL.
    search_tools::AVL_Tree<int> left_heavy;
    for (int key : {3, 2, 1})
    {
        left_heavy.insert(key);
    }
    EXPECT_EQ(left_heavy.select(1), std::optional<int>{1});
    EXPECT_EQ(left_heavy.select(2), std::optional<int>{2});
    EXPECT_EQ(left_heavy.select(3), std::optional<int>{3});
    EXPECT_EQ(left_heavy.select(4), std::nullopt);
    EXPECT_EQ(left_heavy.count_less(2), 1u);

    // RR.
    search_tools::AVL_Tree<int> right_heavy;
    for (int key : {1, 2, 3})
    {
        right_heavy.insert(key);
    }
    EXPECT_EQ(right_heavy.select(1), std::optional<int>{1});
    EXPECT_EQ(right_heavy.select(2), std::optional<int>{2});
    EXPECT_EQ(right_heavy.select(3), std::optional<int>{3});
    EXPECT_EQ(right_heavy.select(4), std::nullopt);
    EXPECT_EQ(right_heavy.count_less(2), 1u);
}

TEST(AVLTreeTest, Double_Rotations)
{
    // LR.
    search_tools::AVL_Tree<int> left_right;
    for (int key : {3, 1, 2})
    {
        left_right.insert(key);
    }
    EXPECT_EQ(left_right.select(1), std::optional<int>{1});
    EXPECT_EQ(left_right.select(2), std::optional<int>{2});
    EXPECT_EQ(left_right.select(3), std::optional<int>{3});
    EXPECT_EQ(left_right.select(4), std::nullopt);
    EXPECT_EQ(left_right.count_less(2), 1u);

    // RL.
    search_tools::AVL_Tree<int> right_left;
    for (int key : {1, 3, 2})
    {
        right_left.insert(key);
    }
    EXPECT_EQ(right_left.select(1), std::optional<int>{1});
    EXPECT_EQ(right_left.select(2), std::optional<int>{2});
    EXPECT_EQ(right_left.select(3), std::optional<int>{3});
    EXPECT_EQ(right_left.select(4), std::nullopt);
    EXPECT_EQ(right_left.count_less(2), 1u);
}
