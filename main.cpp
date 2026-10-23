#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <unistd.h>

#include <include/threaded_avl_tree.hpp>

int main()
{
    search_tools::AVL_Tree<int> a;
    for(int i = 1; i < 15; i++)
    {
        a.insert(i);
    }

    auto result = a.select(1);

    if (result)
    {
        std::cout << *result << '\n';
    }
    else
    {
        std::cout << "Элемента нет\n";
    }
    a.draw();
    return 0;
}