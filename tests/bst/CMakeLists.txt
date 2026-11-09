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


add_library(
  bst
  ../../src/bst.c
)
target_include_directories(bst PRIVATE ../../src/)

add_executable(
  bst_test
  bst_test.cc
)
target_link_libraries(
  bst_test
  GTest::gtest_main
  bst

)

include(GoogleTest)
gtest_discover_tests(bst_test)
