# Project: prekladac-ifj26
# File: CMakeLists.txt
# Author: Martin Vaško (xvaskom00)

cmake_minimum_required(VERSION 3.14)
project(prekladac-ifj26)

# GoogleTest requires C++17 or more
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
)

# Something that needs to be here for Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# library for BST
add_library(
  bst
  ../../src/bst.c
)
target_include_directories(bst PRIVATE ../../src/)

# library for symbol table
add_library(
  symtable
  ../../src/symtable.c
)
target_include_directories(symtable PRIVATE ../../src/)

add_executable(
  symtable_test
  symtable_test.cc
)
target_link_libraries(
  symtable_test
  GTest::gtest_main
  symtable
  bst
)

include(GoogleTest)
gtest_discover_tests(symtable_test)

# conversion of void pointers is ok!
target_compile_options(symtable_test PRIVATE -fpermissive)
