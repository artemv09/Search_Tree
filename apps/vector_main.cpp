#include "sorted_vector.hpp"
#include "command_runner.hpp"

int main()
{
    search_tools::Sorted_Vector<int> container;
    return search_tree_apps::run_commands(container);
}
