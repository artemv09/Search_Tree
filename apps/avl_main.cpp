#include "threaded_avl_tree.hpp"
#include "command_runner.hpp"

int main()
{
    search_tools::AVL_Tree<int> container;
    return search_tree_apps::run_commands(container);
}
