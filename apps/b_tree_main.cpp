#include "b_tree.hpp"
#include "command_runner.hpp"

int main()
{
    search_tools::B_Tree<int> container(4);
    return search_tree_apps::run_commands(container);
}
