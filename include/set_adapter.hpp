#ifndef SET_ADAPTER_
#define SET_ADAPTER_

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <algorithm>
#include <optional>
#include <limits>
#include <cassert>
#include <set>
#include <utility>
#include <stdexcept>

#include "input.hpp"

namespace search_tools {

template <typename Type_Val>
class Set_Adapter
{
    private:
        using Set = std::set<Type_Val>;
        using Set_Iterator = typename Set::iterator;

        Set data_;
        
    public:
        [[nodiscard]] bool execution_command(const input::Parsed_Command& command, std::ostream& results);

        // Вставить ключ
        void insert(Type_Val val);

        // Число ключей в (l, r]
        [[nodiscard]] std::size_t range_count(const Type_Val& l, const Type_Val& r) const;

        // i-й ключ по возрастанию
        [[nodiscard]] std::optional<Type_Val> select(std::size_t i) const;

        // Число ключей строго меньше x
        [[nodiscard]] std::size_t count_less(const Type_Val& x) const;

        Set_Adapter() = default;
};

template <typename Type_Val>
[[nodiscard]] bool Set_Adapter<Type_Val>::execution_command(const input::Parsed_Command& command, std::ostream& results)
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
            results << range_count(Type_Val(command.argument), Type_Val(command.right_bound))// << ' ';
            break;
        }
        case(input::Command::Select):
        {
            const std::optional<Type_Val> value = select(command.index_one_based);
            if (!value)
            {
                return false;
            }
            results << *value;// << ' ';
            break;
        }
        case(input::Command::CountLess):
        {
            results << count_less(Type_Val(command.argument))// << ' ';
            break;
        }
        default:
        {
            throw std::invalid_argument("неизвестный тип команды");
        }
    }

    if (!results)
    {
        throw std::ios_base::failure("ошибка вывода результата команды");
    }
    return true;
}
    
template <typename Type_Val>
void Set_Adapter<Type_Val>::insert(Type_Val val)
{
    data_.insert(std::move(val));
}

template <typename Type_Val>
[[nodiscard]] std::size_t Set_Adapter<Type_Val>::range_count(const Type_Val& l, const Type_Val& r) const
{
    const auto comp = data_.key_comp();
    if (comp(r, l))
    {
        throw std::invalid_argument("обратный диапазон");
    }
    if (!comp(l, r))
    {
        return 0;
    }

    return static_cast<std::size_t>(std::distance(data_.upper_bound(l), data_.upper_bound(r)));
}

template <typename Type_Val>
[[nodiscard]] std::optional<Type_Val> Set_Adapter<Type_Val>::select(std::size_t i) const
{
    if (i == 0 || i > data_.size())
    {
        return std::nullopt;
    }

    Set_Iterator iterator = data_.cbegin();
    for (std::size_t position = 1; position < i; position++)
    {
        iterator++;
    }
    return *iterator;
}

template <typename Type_Val>
[[nodiscard]] std::size_t Set_Adapter<Type_Val>::count_less(const Type_Val& x) const
{
    return static_cast<std::size_t>(std::distance(data_.cbegin(), data_.lower_bound(x)));
}

} //namespace search_tools



#endif
