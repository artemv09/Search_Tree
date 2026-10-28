#ifndef B_TREE_
#define B_TREE_

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
#include <memory>
#include <stdexcept>
#include <utility>
#include <filesystem>

#include "input.hpp"
#include "tree_graphviz.hpp"


namespace search_tools {

template <typename Type_Val>
class B_Tree
{
    private:
        struct Node
        {
            std::vector<Type_Val> value_;
            std::vector<std::unique_ptr<Node>> children_;

            Node* parent_ = nullptr;
        
            std::size_t subtree_size_;

            explicit Node(std::size_t t, const Type_Val& val) : subtree_size_(1)
            {
                value_.reserve(2 * t - 1);
                children_.reserve(2 * t);

                value_.push_back(val);
            }

            explicit Node(std::size_t t) : subtree_size_(0)
            {
                value_.reserve(2 * t - 1);
                children_.reserve(2 * t);
            }
        };

        std::unique_ptr<Node> root_ = nullptr;
        std::size_t t;

        void split_child(Node& parent, std::size_t child_index); //инвариант что parents не полный

        [[nodiscard]] bool insert_not_full(Node& node, const Type_Val& val);

        std::size_t count_less_equal(Type_Val x) const;

    public:
        explicit B_Tree(std::size_t t) : t(t)
        {
            if (t < 2)
            {
                throw std::invalid_argument("недопустимая минимальная степень B-дерева");
            }
        }

        ~B_Tree() = default;

        [[nodiscard]] bool execution_command(const input::Parsed_Command& command, std::ostream& results);

        void insert(Type_Val val);

        [[nodiscard]] std::size_t range_count(const Type_Val& l, const Type_Val& r) const;

        [[nodiscard]] std::optional<Type_Val> select(std::size_t i) const;

        [[nodiscard]] std::size_t count_less(const Type_Val& x) const;

        void draw() const
        {
            graphiviz::draw_b_tree(root_.get());
        }
};

template <typename Type_Val>
[[nodiscard]] bool B_Tree<Type_Val>::execution_command(const input::Parsed_Command& command, std::ostream& results)
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

template <typename Type_Val>
void B_Tree<Type_Val>::insert(Type_Val val)
{
    if (root_ == nullptr)
    {
        root_ = std::make_unique<Node>(t, val);
        return;
    }
    if(root_ -> value_.size() == 2 * t - 1)
    {
        auto new_root = std::make_unique<Node>(t);

        new_root -> subtree_size_ = root_ -> subtree_size_;

        new_root -> children_.push_back(std::move(root_));
        new_root -> children_.back() -> parent_ = new_root.get();

        root_ = std::move(new_root);
    
        split_child(*root_, 0);
    }

    static_cast<void>(insert_not_full(*root_, val));
}

template <typename Type_Val>
[[nodiscard]] std::size_t B_Tree<Type_Val>::range_count(const Type_Val& l, const Type_Val& r) const
{
    if (root_ == nullptr)
    {
        return 0;
    }

    if (r < l)
    {
        throw std::invalid_argument("обратный диапазон");
    }

    if (!(l < r))
    {
        return 0;
    }

    return count_less_equal(r) - count_less_equal(l);
}

template <typename Type_Val>
[[nodiscard]] std::optional<Type_Val> B_Tree<Type_Val>::select(std::size_t i) const
{
   if (root_ == nullptr || i == 0 || i > root_ -> subtree_size_)
    {
        return std::nullopt;
    }

    const Node* node = root_.get();

    while (!node -> children_.empty())
    {
        std::size_t key_index = 0;
        std::size_t size_val = node -> value_.size();

        for (; key_index < size_val; ++key_index)
        {
            const std::size_t child_size = node -> children_.at(key_index) -> subtree_size_;

            if (i <= child_size)
            {
                break;
            }

            i -= child_size;

            if (i == 1)
            {
                return node -> value_.at(key_index);
            }
            --i;
        }

        node = node -> children_.at(key_index).get();
    }

    return node -> value_.at(i - 1); //дошли до листа
}

template <typename Type_Val>
[[nodiscard]] std::size_t B_Tree<Type_Val>::count_less(const Type_Val& x) const
{
    if (root_ == nullptr)
    {
        return 0;
    }

    const Node* node = root_.get();
    std::size_t count_min_ell = 0;

    while (!node -> children_.empty())
    {
        std::size_t key_index = 0;
        std::size_t size_val = node -> value_.size();

        for (; key_index < size_val; ++key_index)
        {
            const std::size_t child_size = node -> children_.at(key_index) -> subtree_size_;

            if (x <= node -> value_.at(key_index))
            {
                break;
            }

            count_min_ell += child_size + 1;

        }

        node = node -> children_.at(key_index ).get();
    }

    const auto position = std::lower_bound(node -> value_.begin(), node -> value_.end(), x);
    count_min_ell += static_cast<std::size_t>(position - node -> value_.begin());

    return count_min_ell; //дошли до листа
}

template <typename Type_Val>
void B_Tree<Type_Val>::split_child(Node& parent, std::size_t child_index) //инвариант что parents не полный
{
    Node* node_child = parent.children_.at(child_index).get();

    auto right_child = std::make_unique<Node>(t);

    right_child -> parent_ = &parent;
    right_child -> value_.assign(node_child -> value_.begin() + t, node_child -> value_.end());

    parent.value_.insert(parent.value_.begin() + child_index, node_child -> value_[t - 1]);
    node_child -> value_.erase(node_child -> value_.begin() + t - 1, node_child -> value_.end());

    if (!node_child -> children_.empty())
    {
        std::size_t size = node_child -> children_.size();
        for (std::size_t i = t; i < size; ++i)
        {
            right_child -> children_.push_back(std::move(node_child -> children_[i]));
            right_child -> children_.back() -> parent_ = right_child.get();
        }
        node_child -> children_.erase(node_child -> children_.begin() + t, node_child -> children_.end());
    }

    node_child -> subtree_size_ = node_child -> value_.size();

    for (const auto& child : node_child -> children_)
    {
        node_child -> subtree_size_ += child -> subtree_size_;
    }

    right_child -> subtree_size_ = right_child -> value_.size();

    for (const auto& child : right_child -> children_)
    {
        right_child -> subtree_size_ += child -> subtree_size_;
    }

    parent.children_.insert(parent.children_.begin() + child_index + 1, std::move(right_child));

}   

template <typename Type_Val>
[[nodiscard]] bool B_Tree<Type_Val>::insert_not_full(Node& node, const Type_Val& val)
{
    const auto it_insertion_point = std::lower_bound(node.value_.begin(), node.value_.end(), val);

    if (it_insertion_point != node.value_.end() && !(val < *it_insertion_point))
    {
        return false;
    }

    if (node.children_.empty())
    {
        node.value_.insert(it_insertion_point, val);
        ++node.subtree_size_;
        return true;
    }

    std::size_t child_index = static_cast<std::size_t>(it_insertion_point - node.value_.begin());

    if (node.children_.at(child_index) -> value_.size() == 2 * t - 1)
    {
        split_child(node, child_index);
        
        if (node.value_.at(child_index) < val)
        {
            ++child_index;
        }
        else if (val == node.value_.at(child_index))
        {
            return false;
        }
    }

    if (!insert_not_full(*node.children_.at(child_index), val))
    {
        return false;
    }

    ++node.subtree_size_;
    return true;
}

template <typename Type_Val>
std::size_t B_Tree<Type_Val>::count_less_equal(Type_Val x) const
{
    if (root_ == nullptr)
    {
        return 0;
    }

    const Node* node = root_.get();
    std::size_t count_min_ell = 0;

    while (!node -> children_.empty())
    {
        std::size_t key_index = 0;
        std::size_t size_val = node -> value_.size();

        for (; key_index < size_val; ++key_index)
        {
            const std::size_t child_size = node -> children_.at(key_index) -> subtree_size_;

            if (x < node -> value_.at(key_index))
            {
                break;
            }

            count_min_ell += child_size + 1;

        }

        node = node -> children_.at(key_index ).get();
    }

    const auto position = std::upper_bound(node -> value_.begin(), node -> value_.end(), x);
    count_min_ell += static_cast<std::size_t>(position - node -> value_.begin());

    return count_min_ell; //дошли до листа
}

} //search_tools

#endif
