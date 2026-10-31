#ifndef SORTED_VECTOR_
#define SORTED_VECTOR_

#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "input.hpp"

namespace search_tools {

template <typename Type_Val, typename Compare = std::less<Type_Val>>
class Sorted_Vector
{
    private:
        using Sorted_Vector_Iterator = typename std::vector<Type_Val>::const_iterator;

        std::vector<Type_Val> data_;
        Compare comp_;

    public:
        [[nodiscard]] bool execution_command(const input::Parsed_Command& command, std::ostream& results);

        void insert(Type_Val val);

        [[nodiscard]] std::size_t range_count(const Type_Val& l, const Type_Val& r) const;

        [[nodiscard]] std::optional<Type_Val> select(std::size_t i) const;

        [[nodiscard]] std::size_t count_less(const Type_Val& x) const;

        Sorted_Vector(Compare comp = Compare{}) : comp_(std::move(comp)) {}
};

template <typename Type_Val, typename Compare>
[[nodiscard]] bool Sorted_Vector<Type_Val, Compare>::execution_command(const input::Parsed_Command& command, std::ostream& results)
{
    switch (command.type)
    {
        case(input::Command::Insert):
        {
            insert(Type_Val(command.argument));
            return true;
        }
        case(input::Command::RangeCount):
        {
            results << range_count(Type_Val(command.argument), Type_Val(command.right_bound));
            break;
        }
        case(input::Command::Select):
        {
            const std::optional<Type_Val> value = select(command.index_one_based);
            if (!value)
            {
                return false;
            }
            results << *value;
            break;
        }
        case(input::Command::CountLess):
        {
            results << count_less(Type_Val(command.argument));
            break;
        }
        default:
        {
            throw std::invalid_argument("неизвестный тип команды");
        }
    }

    return true;
}

template <typename Type_Val, typename Compare>
void Sorted_Vector<Type_Val, Compare>::insert(Type_Val val)
{
    const auto it_insertion_point = std::lower_bound(data_.begin(), data_.end(), val, comp_);

    if (it_insertion_point != data_.end() && !comp_(val, *it_insertion_point))
    {
        return;
    }

    data_.insert(it_insertion_point, std::move(val));
}

template <typename Type_Val, typename Compare>
[[nodiscard]] std::size_t Sorted_Vector<Type_Val, Compare>::range_count(const Type_Val& l, const Type_Val& r) const
{
    if (comp_(r, l))
    {
        throw std::invalid_argument("обратный диапазон");
    }
    if (!comp_(l, r))
    {
        return 0;
    }

    const auto it_left = std::upper_bound(data_.cbegin(), data_.cend(), l, comp_);
    const auto it_right = std::upper_bound(data_.cbegin(), data_.cend(), r, comp_);
    return static_cast<std::size_t>(it_right - it_left);
}

template <typename Type_Val, typename Compare>
[[nodiscard]] std::optional<Type_Val> Sorted_Vector<Type_Val, Compare>::select(std::size_t i) const
{
    if (i == 0 || i > data_.size())
    {
        return std::nullopt;
    }
    return data_[i - 1];
}

template <typename Type_Val, typename Compare>
[[nodiscard]] std::size_t Sorted_Vector<Type_Val, Compare>::count_less(const Type_Val& x) const
{
    const auto it_position = std::lower_bound(data_.cbegin(), data_.cend(), x, comp_);
    return static_cast<std::size_t>(it_position - data_.cbegin());
}

}

#endif
