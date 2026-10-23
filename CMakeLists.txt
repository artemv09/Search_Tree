cmake_minimum_required(VERSION 3.16)
project(Search_Tree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(search_tree
    main.cpp
    src/input.cpp
    include/input.hpp
    include/set_adapter.hpp
    include/sorted_vector.hpp
    include/threaded_avl_tree.hpp
    include/b_tree.hpp
    graphiviz/tree_graphviz.hpp
)

target_include_directories(search_tree PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/graphiviz"
)
