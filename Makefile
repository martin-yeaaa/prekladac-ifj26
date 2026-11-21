# Project: prekladac-ifj26
# File: Makefile
# Author: Martin Vaško (xvaskom00)

CC = gcc
CFLAGS = -std=c23 -pedantic -Wall -Wextra -Werror

MAIN = src/main.c src/error_codes.h
LEXER = src/lexer.c src/lexer.h
PARSER = src/parser.c src/parser.h
SEMANTICS = src/semantics.c src/semantics.h src/semantics_priv.h
SYMTABLE = src/symtable.c src/symtable.h
BST = src/bst.c src/bst.h src/bst_priv.h
#CODEGEN = TODO

SOURCES = $(MAIN) $(LEXER) $(PARSER) $(SEMANTICS) $(SYMTABLE) $(BST)

all: debug_build

debug_build: $(SOURCES)
	$(CC) $(CFLAGS) $^ -o prekladac-ifj26

permissive: $(SOURCES)
	$(CC) -std=c23 $^ -o prekladac-ifj26


format:
	clang-format -i src/*

