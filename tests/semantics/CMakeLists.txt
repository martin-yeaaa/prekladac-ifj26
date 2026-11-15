# CMakeLists.txt for testing
# Author: Martin Vaško

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
# library for semantics module
add_library(
  semantics
  ../../src/semantics.c
)
target_include_directories(semantics PRIVATE ../../src/)


add_executable(
  semantics_test
  semantics_test.cc
)
target_link_libraries(
  semantics_test
  GTest::gtest_main
  bst
  symtable
  semantics
)

include(GoogleTest)
gtest_discover_tests(semantics_test)

# C compatibility with C++ tests
# allows implicit conversion of void pointers
target_compile_options(semantics_test PRIVATE -fpermissive -w)
# allows string literal in char *
target_compile_options(semantics_test PRIVATE -Wno-write-strings)