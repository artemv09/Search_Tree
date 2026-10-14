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

namespace search_tools {

template <typename Type_Val>
class Set_Adapter
{
    private:
        using Set = std::set<Type_Val>;
        using Set_Iterator = typename Set::iterator;

        Set data_;


        
    public:

        [[nodiscard]] bool execution_command();
        // вставить эллемент
        void insert(Type_Val val);

        // Количество элементов в [l, r]
        [[nodiscard]] std::size_t range_count(int l, int r) const;

        // i-й элемент по возрастанию, начиная с 1
        [[nodiscard]] std::optional<Type_Val> select(std::size_t i) const;

        // Количество элементов строго меньше x
        [[nodiscard]] std::size_t count_less(int x) const;

        Set_Adapter() = defult;
};

template <typename Type_Val>
[[nodiscard]] bool Set_Adapter<Type_Val>::execution_command()
{

}
    
template <typename Type_Val>
void Set_Adapter<Type_Val>::insert(Type_Val val)
{

}

template <typename Type_Val>
[[nodiscard]] std::size_t Set_Adapter<Type_Val>::range_count(int l, int r) const
{

}

template <typename Type_Val>
[[nodiscard]] std::optional<Type_Val> Set_Adapter<Type_Val>::select(std::size_t i) const
{

}

template <typename Type_Val>    
[[nodiscard]] std::size_t Set_Adapter<Type_Val>::count_less(int x) const
{

}

} //namespace search_tools



#endif