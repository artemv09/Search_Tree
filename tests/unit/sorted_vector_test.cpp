#include <gtest/gtest.h>
#include <functional>
#include <optional>
#include <stdexcept>

#include "sorted_vector.hpp"

TEST(SortedVectorTest, Empty_Select)
{
    search_tools::Sorted_Vector<int> container;

    EXPECT_EQ(container.select(0), std::nullopt);
    EXPECT_EQ(container.select(1), std::nullopt);
    EXPECT_EQ(container.count_less(2), 0u);
    EXPECT_EQ(container.range_count(-1, 8), 0u);
    EXPECT_EQ(container.range_count(2, 2), 0u);
}

TEST(SortedVectorTest, Insert_One)
{
    search_tools::Sorted_Vector<int> container;
    container.insert(2);

    EXPECT_EQ(container.select(1), std::optional<int>{2});
    EXPECT_EQ(container.select(0), std::nullopt);
    EXPECT_EQ(container.select(2), std::nullopt);
}

TEST(SortedVectorTest, Unsorted_Insert)
{
    search_tools::Sorted_Vector<int> container;

    for (int key : {8, -1, 2})
    {
        container.insert(key);
    }

    EXPECT_EQ(container.select(1), std::optional<int>{-1});
    EXPECT_EQ(container.select(2), std::optional<int>{2});
    EXPECT_EQ(container.select(3), std::optional<int>{8});
    EXPECT_EQ(container.select(4), std::nullopt);
}

TEST(SortedVectorTest, No_Duplicates)
{
    search_tools::Sorted_Vector<int> container;
    container.insert(2);
    container.insert(2);

    EXPECT_EQ(container.select(1), std::optional<int>{2});
    EXPECT_EQ(container.select(2), std::nullopt);
    EXPECT_EQ(container.count_less(3), 1u);
}

TEST(SortedVectorTest, Count_Less)
{
    search_tools::Sorted_Vector<int> container;

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

TEST(SortedVectorTest, Range_Bounds)
{
    search_tools::Sorted_Vector<int> container;

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

TEST(SortedVectorTest, Reverse_Range)
{
    search_tools::Sorted_Vector<int> container;

    EXPECT_THROW((void)container.range_count(10, 5), std::invalid_argument);

    container.insert(2);

    EXPECT_THROW((void)container.range_count(10, 5), std::invalid_argument);
}

TEST(SortedVectorTest, Descending_Order)
{
    search_tools::Sorted_Vector<int, std::greater<int>> container;

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
