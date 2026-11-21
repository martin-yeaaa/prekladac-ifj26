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
#include <string.h>

ErrCode compile(void) {

  ErrCode err = SUCCESS;

  // Reads stdin, creates all tokens at once
  // lexer_parse MAY EXIT!!!
  Token *token_list = lexer_parse();

  // Abstract syntax tree = AST
  ASTNode program_tree;
  err = AST_init_node(&program_tree, NODE_PROGRAM, DT_INTEGER, NULL);
  if (err != SUCCESS) {
    lexer_free_tokens(token_list);
    fprintf(stderr, "AST initialization failed.\n");
    return err;
  }

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
    ST_destroy(&st);
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

static ErrCode parse_program(ASTNode *ast_root) {

  Token *token = get_next_token();

  // Match rule
  switch (token->type) {
  case KW_IMPORT: {

    // <program> -> <prolog> <fun_def_list>
    // <prolog> -> import ifj26.*
    ErrCode err1 = parse_prolog(ast_root);
    if (err1 != SUCCESS)
      return err1;
    ErrCode err2 = parse_fun_def_list(ast_root);
    if (err2 != SUCCESS)
      return err2;

    return SUCCESS;
  }
  default:
    break;
  }

  return ERR_SYNTAX;
}

static ErrCode parse_prolog(ASTNode *ast_root) {

  Token *token_1 = get_next_token();

  // Match rule
  switch (token_1->type) {
  case IDENTIFIER: {

    // <prolog> -> import ifj26.*
    // expected ifj26.* in tokens is identifier, dot, op_mult
    // we do a little bit of workaround here
    if (strcmp(token_1->value, "ifj26") == 0) {

      Token *token_2 = get_next_token();
      Token *token_3 = get_next_token();

      if (token_2->type == DOT && token_3->type == OP_MULT) {
        return SUCCESS;
      }
      // else falls through to error
    }
    // else falls through to error
  }
  default:
    break;
  }

  return ERR_SYNTAX;
}

static ErrCode parse_fun_def_list(ASTNode *ast_root) {

  Token *token = get_next_token();

  // Match rule
  switch (token->type) {
  case NEW_LINE: {

    // TODO ADD TO DOCUMENTATION I GUESS
    // There always seems to be a new line character before end of file
    if (token->next != NULL) {
      if (token->next->type == END_OF_FILE) {
        return SUCCESS;
      }
    }

    // <fun_def_list> -> \NL <fun_def_any> <fun_def_list>
    ErrCode err1 = parse_fun_def_any(ast_root);
    if (err1 != SUCCESS)
      return err1;
    ErrCode err2 = parse_fun_def_list(ast_root);
    if (err2 != SUCCESS)
      return err2;

    return SUCCESS;
  }
  case END_OF_FILE: {

    //<fun_def_list> -> epsilon
    return SUCCESS;
  }
  default:
    break;
  }

  return ERR_SYNTAX;
}

static ErrCode parse_fun_def_any(ASTNode *ast_root) {

  // TODO
  return SUCCESS;
}