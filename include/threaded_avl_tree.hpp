#ifndef AVL_TREE_
#define AVL_TREE_

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


namespace search_tools {

template <typename Type_Val>
class AVL_Tree
{
    private:

        enum class Search_Status
        {
            Found,        // Ключ найден
            Insert_Left,   // Нужно вставить слева от найденного родителя
            Insert_Right,  // Нужно вставить справа от найденного родителя
            Empty_Tree     // Дерево пустое — новый узел станет корнем
        };

        enum class Link_Type
        {
            Child,
            Thread
        };

        struct Node
        {
            Type_Val value_;  // Ключ, который хранит этот узел

            Node* left_   = nullptr;
            Node* right_  = nullptr;
            Node* parent_ = nullptr;

            Link_Type left_type_  = Link_Type::Thread;
            Link_Type right_type_ = Link_Type::Thread;

            int height_ = 0;
            std::size_t subtree_size_ = 1;  //кол во поддеревье включь его самого

            explicit Node(Type_Val&& value): value_(std::move(value))
            {
            }
        };

        struct Search_Result
        {
            Node* node;
            Search_Status status;
        };

        Node* root_ = nullptr;

        Search_Result find_position(const Type_Val& value);

    public:
        [[nodiscard]] bool execution_command(const input::Parsed_Command& command, std::ostream& results);

        void insert(Type_Val val);

        [[nodiscard]] std::size_t range_count(const Type_Val& l, const Type_Val& r) const;

        [[nodiscard]] std::optional<Type_Val> select(std::size_t i) const;

        [[nodiscard]] std::size_t count_less(const Type_Val& x) const;

        AVL_Tree() = default;

        ~AVL_Tree()
        {
            
        }
};

template <typename Type_Val>
void AVL_Tree<Type_Val>::insert(Type_Val val)
{
    Search_Result find_pos = find_position(val);

    switch(find_pos.status)
    {
        case(Search_Status::Empty_Tree):
        {
            root_ = std::make_unique<Node>(std::move(val));
        }
        case(Search_Status::Found):
        {
            break;
        }
        case(Search_Status::Insert_Left):
        {
            
        }
        case(Search_Status::Insert_Right):
        {
            
        }
    }
}

template <typename Type_Val>
typename AVL_Tree<Type_Val>::Search_Result AVL_Tree<Type_Val>::find_position(const Type_Val& value)
{
    if (root_ == nullptr)
    {
        return {nullptr, Search_Status::Empty_Tree};
    }

    Node* current = root_;

    while(true)
    {
        if (value < current -> value_)
        {
            if (current -> left_type_ == Link_Type::Thread)
            {
                return {current, Search_Status::Insert_Left};
            }
            current = current -> left_;
        }
        else if (current -> value_ < value)
        {
            if (current -> right_type_ == Link_Type::Thread)
            {
                return {current, Search_Status::Insert_Right};
            }
            current = current -> right_;
        }
        else
        {
            return {current, Search_Status::Found};
        }
    }
}

} //search_tools



#endif
