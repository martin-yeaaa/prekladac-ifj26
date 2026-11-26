# Makefile for testing
# Author: Martin Vaško

# this make target will compile and run tests
testing:
	cmake -S . -B build
	cmake --build build
	cd build && ctest


# Runs valgrind on googletest tests, only with Fixture somewhere in their test suite name.
memleak:
	valgrind --leak-check=full ./build/semantics_test --gtest_filter=*Fixture*