#include "set_adapter.hpp"
#include "command_runner.hpp"

int main()
{
    search_tools::Set_Adapter<int> container;
    return search_tree_apps::run_commands(container);
}
