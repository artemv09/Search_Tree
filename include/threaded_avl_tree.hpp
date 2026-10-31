#ifndef AVL_TREE_
#define AVL_TREE_

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <functional>
#include <algorithm>
#include <optional>
#include <limits>
#include <cassert>
#include <memory.h>
#include <utility>

#include "input.hpp"
#include "tree_graphviz.hpp"


namespace search_tools {

template <typename Type_Val, typename Compare = std::less<Type_Val>>
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
            std::size_t subtree_size_ = 1;

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
        Compare comp_;

        Search_Result find_position(const Type_Val& value) const;

        // Обновляет высоту и размер одного узла по данным его детей.
        void update_metadata(Node* node);

        // Возвращает: высота справа минус высота слева.
        int balance_factor(const Node* node) const;

        // Выполняют поворот и возвращают новый корень поддерева.
        Node* rotate_left(Node* node);
        Node* rotate_right(Node* node);

        // Проверяет один узел и выбирает необходимые повороты.
        Node* rebalance(Node* node);

        // Обновляет и балансирует узлы на пути к корню.
        void rebalance_up(Node* node);

    public:
        [[nodiscard]] bool execution_command(const input::Parsed_Command& command, std::ostream& results);

        void insert(Type_Val val);

        [[nodiscard]] std::size_t range_count(const Type_Val& l, const Type_Val& r) const;

        [[nodiscard]] std::optional<Type_Val> select(std::size_t i) const;

        [[nodiscard]] std::size_t count_less(const Type_Val& x) const;

        AVL_Tree(Compare comp = Compare{}) : comp_(std::move(comp)) {}

        ~AVL_Tree();

        void draw() const
        {
            graphiviz::draw_tree(root_);
        }
};

template <typename Type_Val, typename Compare>
void AVL_Tree<Type_Val, Compare>::insert(Type_Val val)
{
    Search_Result find_pos = find_position(val);

    switch(find_pos.status)
    {
        case(Search_Status::Empty_Tree):
        {
            auto temporary_root = std::make_unique<Node>(std::move(val));
            root_ = temporary_root.get();
            temporary_root.release();

            break;
        }
        case(Search_Status::Found):
        {
            break;
        }
        case (Search_Status::Insert_Left):
        {
            Node* parent = find_pos.node;

            auto temporary_node = std::make_unique<Node>(std::move(val));
            Node* new_node = temporary_node.get();

            new_node -> parent_ = parent;

            new_node -> left_ = parent -> left_; 
            new_node -> right_ = parent;

            parent -> left_ = new_node;
            parent -> left_type_ = Link_Type::Child;

            temporary_node.release();

            rebalance_up(new_node);
            break;
        }

        case (Search_Status::Insert_Right):
        {
            Node* parent = find_pos.node;

            auto temporary_node = std::make_unique<Node>(std::move(val));
            Node* new_node = temporary_node.get();

            new_node -> parent_ = parent;

            new_node -> left_ = parent;
            new_node -> right_ = parent -> right_; 

            parent -> right_ = new_node;
            parent -> right_type_ = Link_Type::Child;

            temporary_node.release();

            rebalance_up(new_node);
            break;
        }
    }
}

template <typename Type_Val, typename Compare>
typename AVL_Tree<Type_Val, Compare>::Search_Result AVL_Tree<Type_Val, Compare>::find_position(const Type_Val& value) const
{
    if (root_ == nullptr)
    {
        return {nullptr, Search_Status::Empty_Tree};
    }

    Node* current = root_;

    while(true)
    {
        if (comp_(value, current -> value_))
        {
            if (current -> left_type_ == Link_Type::Thread)
            {
                return {current, Search_Status::Insert_Left};
            }
            current = current -> left_;
        }
        else if (comp_(current -> value_, value))
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

template <typename Type_Val, typename Compare>
void AVL_Tree<Type_Val, Compare>::update_metadata(Node* node)
{
    if (node == nullptr)
    {
        return;
    }

    int left_height = -1;
    int right_height = -1;

    std::size_t left_size = 0;
    std::size_t right_size = 0;

    if (node -> left_type_ == Link_Type::Child)
    {
        left_height = node -> left_ -> height_;
        left_size = node -> left_ -> subtree_size_;
    }

    if (node -> right_type_ == Link_Type::Child)
    {
        right_height = node -> right_ -> height_;
        right_size = node -> right_ -> subtree_size_;
    }

    node -> height_ = 1 + std::max(left_height, right_height);
    node -> subtree_size_ = 1 + left_size + right_size;
}

template <typename Type_Val, typename Compare>
int AVL_Tree<Type_Val, Compare>::balance_factor(const Node* node) const
{
    int left_height = -1;
    int right_height = -1;

    if (node -> left_type_ == Link_Type::Child)
    {
        left_height = node -> left_ -> height_;
    }

    if (node -> right_type_ == Link_Type::Child)
    {
        right_height = node -> right_ -> height_;
    }

    return right_height - left_height;
}

template <typename Type_Val, typename Compare>
typename AVL_Tree<Type_Val, Compare>::Node* AVL_Tree<Type_Val, Compare>::rotate_left(Node* node)
{
    Node* new_root = node -> right_;
    Node* old_parent = node -> parent_;

    if (new_root -> left_type_ == Link_Type::Child)
    {
        node -> right_ = new_root -> left_;
        node -> right_type_ = Link_Type::Child;

        node -> right_ -> parent_ = node;
    }
    else
    {
        node -> right_ = new_root;
        node -> right_type_ = Link_Type::Thread;
    }

    new_root -> left_ = node;
    new_root -> left_type_ = Link_Type::Child;

    node -> parent_ = new_root;
    new_root -> parent_ = old_parent;

    // Подключаем новый корень поддерева к остальному дереву.
    if (old_parent == nullptr)
    {
        root_ = new_root;
    }
    else if (old_parent -> right_type_ == Link_Type::Child &&
             old_parent -> right_ == node)
    {
        old_parent -> right_ = new_root;
    }
    else
    {
        old_parent -> left_ = new_root;
    }

    update_metadata(node);
    update_metadata(new_root);

    return new_root;
}

template <typename Type_Val, typename Compare>
typename AVL_Tree<Type_Val, Compare>::Node* AVL_Tree<Type_Val, Compare>::rotate_right(Node* node)
{
    Node* new_root = node -> left_;
    Node* old_parent = node -> parent_;

    if (new_root -> right_type_ == Link_Type::Child)
    {
        node -> left_ = new_root -> right_; 
        node -> left_type_ = Link_Type::Child;

        node -> left_ -> parent_ = node;    
    }
    else
    {
        //создаем пиздюка
        node -> left_ = new_root;
        node -> left_type_ = Link_Type::Thread;
    }

    new_root -> right_ = node;
    new_root -> right_type_ = Link_Type::Child;

    node -> parent_ = new_root;
    new_root -> parent_ = old_parent;

    if (old_parent == nullptr)
    {
        root_ = new_root;
    }
    else if (old_parent -> left_type_ == Link_Type::Child && old_parent -> left_ == node)
    {
        old_parent -> left_ = new_root;
    }
    else
    {
        old_parent -> right_ = new_root;
    }

    update_metadata(node);     
    update_metadata(new_root);  

    return new_root;
}

template <typename Type_Val, typename Compare>
typename AVL_Tree<Type_Val, Compare>::Node* AVL_Tree<Type_Val, Compare>::rebalance(Node* node)
{
    const int balance = balance_factor(node);

    if (balance < -1)
    {
        if (balance_factor(node -> left_) > 0)
        {
            rotate_left(node -> left_);
        }

        return rotate_right(node);
    }

    if (balance > 1)
    {
        if (balance_factor(node -> right_) < 0)
        {
            rotate_right(node -> right_);
        }

        return rotate_left(node);
    }

    return node;
}

template <typename Type_Val, typename Compare>
void AVL_Tree<Type_Val, Compare>::rebalance_up(Node* node)
{
     if (node == nullptr)
    {
        return;
    }

    update_metadata(node);

    Node* new_root = rebalance(node);

    rebalance_up(new_root -> parent_);
}

template <typename Type_Val, typename Compare>
[[nodiscard]] bool AVL_Tree<Type_Val, Compare>::execution_command(const input::Parsed_Command& command, std::ostream& results)
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
[[nodiscard]] std::size_t AVL_Tree<Type_Val, Compare>::range_count(const Type_Val& l, const Type_Val& r) const
{
    if (comp_(r, l))
    {
        throw std::invalid_argument("обратный диапазон");
    }

    if (!comp_(l, r))
    {
        return 0;
    }

    Search_Result begin_row = find_position(l);
    const Node* current = begin_row.node;

    std::size_t count = 0;

    while (current != nullptr && !comp_(r, current -> value_))
    {
        if (comp_(l, current -> value_))
        {
            ++count;
        }
        if (current -> right_type_ == Link_Type::Thread)
        {
            current = current -> right_;
        }
        else
        {
            current = current -> right_;

            while (current -> left_type_ == Link_Type::Child)
            {
                current = current -> left_;
            }
        }
    }

    return count;
}

template <typename Type_Val, typename Compare>
[[nodiscard]] std::optional<Type_Val> AVL_Tree<Type_Val, Compare>::select(std::size_t i) const
{
    if(root_ == nullptr || i == 0 || i > root_ -> subtree_size_)
    {
        return std::nullopt;
    }

    Node* min_key = root_;
    while(min_key -> left_ != nullptr)
    {
        min_key = min_key -> left_;
    }

    std::size_t count = 1;

    while (min_key != nullptr && count < i)
    {
        if (min_key -> right_type_ == Link_Type::Child)
        {
            min_key = min_key -> right_;

            if (min_key -> left_type_ == Link_Type::Thread)
            {
                count++;
            }
            else
            {
                while(min_key -> left_type_ != Link_Type::Thread)
                {
                    min_key = min_key -> left_;
                }
                count++;
            }
        }
        else
        {
            min_key = min_key -> right_;
            count++;
        }
    }

    if (count == i)
    {
        return min_key -> value_;
    }
    else
    {
        return std::nullopt;
    }
}

template <typename Type_Val, typename Compare> // Число ключей строго меньше x
[[nodiscard]] std::size_t AVL_Tree<Type_Val, Compare>::count_less(const Type_Val& x) const
{
    std::size_t count = 0;
    const Node* current = root_;

    while (current != nullptr)
    {
        if (comp_(current -> value_, x))
        {
            ++count;

            if (current -> left_type_ == Link_Type::Child)
            {
                count += current -> left_ -> subtree_size_;
            }

            if (current -> right_type_ == Link_Type::Thread)
            {
                break;
            }

            current = current -> right_;
        }
        else
        {
            if (current -> left_type_ == Link_Type::Thread)
            {
                break;
            }

            current = current -> left_;
        }
    }

    return count;
}

template <typename Type_Val, typename Compare> //освободить все владения root
AVL_Tree<Type_Val, Compare>::~AVL_Tree()
{
    Node* del_key = root_;

    if (del_key == nullptr)
    {
        return;
    }

    while (del_key -> left_type_ == Link_Type::Child)
    {
        del_key = del_key -> left_;
    }

    while (del_key != nullptr)
    {
        Node* next = del_key -> right_;

        if (del_key -> right_type_ == Link_Type::Child)
        {
            while (next -> left_type_ == Link_Type::Child)
            {
                next = next -> left_;
            }
        }

        delete del_key;
        del_key = next;
    }

    root_ = nullptr;
}

} //search_tools


#endif
