#include <gtest/gtest.h>
#include <optional>

#include "sorted_vector.hpp"

TEST(SortedVectorSmoke, EmptySelectReturnsNullopt)
{
    search_tools::Sorted_Vector<int> container;
    EXPECT_EQ(container.select(1), std::nullopt);
}
