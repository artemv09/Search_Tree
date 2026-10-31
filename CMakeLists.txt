cmake_minimum_required(VERSION 3.16)
project(Search_Tree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SEARCH_TREE_BUILD_APPS
    "Build standalone algorithm programs" ON)

option(SEARCH_TREE_BUILD_BENCHMARKS
    "Build research program" OFF)

include(CTest)

add_library(search_tree_containers INTERFACE)

target_include_directories(search_tree_containers INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/graphiviz"
)

target_compile_features(search_tree_containers INTERFACE cxx_std_20)

add_library(search_tree_input STATIC src/input.cpp)

target_include_directories(search_tree_input PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(search_tree_input PUBLIC cxx_std_20)

if(SEARCH_TREE_BUILD_APPS)
    add_executable(search_set apps/set_main.cpp)
    add_executable(search_vector apps/vector_main.cpp)
    add_executable(search_avl apps/avl_main.cpp)
    add_executable(search_btree apps/b_tree_main.cpp)

    foreach(app IN ITEMS search_set search_vector search_avl search_btree)
        target_link_libraries(${app} PRIVATE search_tree_containers search_tree_input)
    endforeach()

endif()

if(BUILD_TESTING)
    include(FetchContent)

    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(search_tree_tests
        tests/unit/b_tree_test.cpp
        tests/unit/input_test.cpp
        tests/unit/set_adapter_test.cpp
        tests/unit/sorted_vector_test.cpp
        tests/unit/awl_tree_test.cpp
    )

    target_link_libraries(search_tree_tests PRIVATE
        search_tree_containers search_tree_input GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(search_tree_tests)

endif()

if(SEARCH_TREE_BUILD_BENCHMARKS)
    add_executable(search_tree_bench
        benchmarks/benchmark_main.cpp
        benchmarks/workloads.cpp
    )
    target_link_libraries(search_tree_bench PRIVATE search_tree_containers)
    
endif()
