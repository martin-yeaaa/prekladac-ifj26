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

add_executable(
  example_test
  example_test.cc
)
target_link_libraries(
  example_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(example_test)
