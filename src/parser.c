// Project: prekladac-ifj26
// File: parser.c
// Author: Martin Vaško (vaskom00)

#include "parser.h"
#include "error_codes.h"
#include "lexer.h"
#include "semantics.h"
#include "symtable.h"
// #include "codegen.h" TODO add this when its made

#include <stdio.h>

ErrCode compile(void) {

  ErrCode err = SUCCESS;

  // Reads stdin, creates all tokens at once
  // lexer_parse MAY EXIT!!!
  Token *token_list = lexer_parse();

  // Abstract syntax tree = AST
  ASTNode program_tree;
  // Syntax analysis, also builds AST
  err = parse_program(&program_tree); // Maybe pointer to ast?

  // Free token list after parsing
  lexer_free_tokens(token_list);
  // Exit on syntax analysis failure
  if (err != SUCCESS) {
    AST_destroy_tree(program_tree);
    fprintf(stderr, "Syntax error.\n");
    return err;
  }

  // Prepare symbol table
  SymbTables st;
  err = ST_init(&st);
  if (err != SUCCESS) {
    AST_destroy_tree(program_tree);
    fprintf(stderr, "Symbol table initialization failed.\n");
    return err;
  }

  // Do semantic analysis, also fills symbol table and AST with symbols
  err = semantic_analysis(program_tree, &st);
  if (err != SUCCESS) {
    AST_destroy_tree(program_tree);
    fprintf(stderr, "Semantic error.\n");
    return err;
  }

  // TODO Generate code (cannot fail?)
  // generate_program(program_tree);

  // Free memory before quitting
  ST_destroy(&st);
  AST_destroy_tree(program_tree);

  // Program compiled!
  return SUCCESS;
}