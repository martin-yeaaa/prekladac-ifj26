# Project: prekladac-ifj26
# File: Makefile
# Author: Martin Vaško (xvaskom00)

# FOR TESTING PURPOSES ONLY!!!

CC = gcc-15 # THIS DOESNT WORK ON EVERY MACHINE, for example MERLIN !!
CFLAGS = -std=c23 -pedantic -Wall -Wextra -Werror

S = ../../src/

# this make target will compile and run tests
googletest:
	cmake -S . -B build
	cmake --build build
	cd build && ctest


test: test.c $(S)/bst.h $(S)/bst.c $(S)/bst_priv.h
	$(CC) $(CFLAGS) $^ -o $@
	./$@