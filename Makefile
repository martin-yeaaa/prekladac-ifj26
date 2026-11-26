# Project: prekladac-ifj26
# File: Makefile
# Author: Martin Vaško (xvaskom00)

CC = gcc
CFLAGS = -std=c23 -pedantic -Wall -Wextra -Werror

MAIN = src/main.c src/error_codes.h
LEXER = src/lexer.c src/lexer.h
PARSER = src/parser.c src/parser.h src/parser_priv.h
EXPR_PARSER = src/expr_parser.c src/expr_parser.h
SEMANTICS = src/semantics.c src/semantics.h src/semantics_priv.h
SYMTABLE = src/symtable.c src/symtable.h
BST = src/bst.c src/bst.h src/bst_priv.h
AST_MODULE = src/ast_module.c src/ast_module.h
CODEGEN = src/codegen.h src/codegen.c

SOURCES = $(MAIN) $(LEXER) $(PARSER) $(EXPR_PARSER) $(SEMANTICS) $(SYMTABLE) $(BST) $(AST_MODULE) $(CODEGEN)

all: release

# -DNDEBUG removes asserts!
release: $(SOURCES)
	$(CC) $(CFLAGS) -DNDEBUG $^ -o prekladac_ifj26_bin

# Compiles with asserts
debug_build: $(SOURCES)
	$(CC) $(CFLAGS) $^ -o prekladac_ifj26_bin

# Compiles without caring about bad programming
permissive: $(SOURCES)
	$(CC) -std=c23 $^ -o prekladac_ifj26_bin

# Formats source files
format:
	clang-format --style=llvm -i src/*
