// Project: prekladac-ifj26
// File: parser.c
// Author: Martin Vaško (vaskom00)

#include "parser.h"
#include "error_codes.h"
#include "lexer.h"
#include "parser_priv.h"
#include "semantics.h"
#include "symtable.h"
// #include "codegen.h" TODO add this when its made

#include <assert.h>
#include <stdio.h>
#include <string.h>

// Macro for checking for errors and returning if error occurred
// Similar to macro from semantics.c, but without error message.
#define IF_ERROR_RETURN(error_code)                                            \
  do {                                                                         \
    ErrCode evaluated_err = (error_code);                                      \
    if (evaluated_err != SUCCESS) {                                            \
      return evaluated_err;                                                    \
    }                                                                          \
  } while (0)

// Global variable that holds current token.
// It's easier to keep track of it this way, otherwise we would need to pass
// a pointer to pointer to token in every parsing function.
static Token *cur_token;

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
  err = parse_program(program_tree); // Maybe pointer to ast?

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

static ErrCode make_child_node(ASTNode parent, ASTNode *ptr_uninit,
                               NodeType n_type, DataType d_type, char *n_data) {

  assert(parent != NULL);
  assert(ptr_uninit != NULL);

  // Initialize new node
  ErrCode err = AST_init_node(ptr_uninit, n_type, d_type, n_data);
  IF_ERROR_RETURN(err);

  // Dereference
  // It's ok because type ASTNode is a pointer to structure on heap
  ASTNode child = *ptr_uninit;

  // Add new node into tree
  err = AST_node_add_child(parent, child);
  if (err != SUCCESS) {
    // If it fails, we need to get rid of the child
    AST_destroy_tree(child);
    return err;
  }

  return SUCCESS;
}

static ErrCode parse_program(ASTNode ast_root) {

  // AST root node is initialized already

  cur_token = get_next_token();
  assert(cur_token != NULL); // NULL means program wasn't tokenized

  // Match rule
  switch (cur_token->type) {
  case KW_IMPORT: {

    // <program> -> <prolog> <fun_def_list>
    // <prolog> -> import ifj26.*
    ErrCode err1 = parse_prolog();
    IF_ERROR_RETURN(err1);

    //
    ErrCode err2 = parse_fun_def_list(ast_root);
    IF_ERROR_RETURN(err2);

    return SUCCESS;
  }
  default:
    break;
  }

  return ERR_SYNTAX;
}

static ErrCode parse_prolog(void) {

  if (cur_token == NULL)
    cur_token = get_next_token();

  // Match rule
  switch (cur_token->type) {
  case KW_IMPORT: {

    // Token was parsed
    cur_token = NULL;

    // <prolog> -> import ifj26.*
    // expected ifj26.* in tokens is identifier, dot, op_mult
    // we do a little bit of workaround here
    Token *token_1 = get_next_token();
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

static ErrCode parse_fun_def_list(ASTNode ast_root) {

  if (cur_token == NULL)
    cur_token = get_next_token();

  // Match rule
  switch (cur_token->type) {
  case NEW_LINE: {

    // TODO ADD TO DOCUMENTATION I GUESS
    // There always seems to be a new line character before end of file
    // So we look ahead!
    if (cur_token->next != NULL) {
      //<fun_def_list> -> epsilon
      if (cur_token->next->type == END_OF_FILE) {
        return SUCCESS;
      }
    }
    cur_token = NULL; // Token was parsed

    // <fun_def_list> -> \NL <fun_def_any> <fun_def_list>
    ErrCode err1 = parse_fun_def_any(ast_root);
    IF_ERROR_RETURN(err1);
    ErrCode err2 = parse_fun_def_list(ast_root);
    IF_ERROR_RETURN(err2);

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

static ErrCode parse_fun_def_any(ASTNode ast_root) {

  if (cur_token == NULL)
    cur_token = get_next_token();

  // Prepare unfinished node for function definition
  ASTNode func_def;
  ErrCode err =
      make_child_node(ast_root, &func_def, NODE_FUNC_DEF, DT_UNDEF, NULL);
  IF_ERROR_RETURN(err);

  // 5 <fun_def_any> -> @main <fun_def>
  // 6 <fun_def_any> -> <fun_def>
  //  Match rule
  switch (cur_token->type) {
  case AT_MAIN: {

    // <fun_def_any> -> @main <fun_def>
    cur_token = NULL;                         // Token was parsed
    func_def->node_type = NODE_MAIN_FUNC_DEF; // It is @main!
    return parse_fun_def(func_def);
  }
  case KW_DEF: {

    // <fun_def_any> -> <fun_def>
    return parse_fun_def(func_def);
  }
  default:
    break;
  }

  return ERR_SYNTAX;
}

static ErrCode parse_fun_def(ASTNode func_def) {

  if (cur_token == NULL)
    cur_token = get_next_token();

  ErrCode err = SUCCESS;

  //  Match rule
  switch (cur_token->type) {
  case KW_DEF: {

    // <fun_def> -> def IDENTIFIER (<param_list>) <data_type_opt> = <fun_body>
    cur_token = get_next_token();
    if (cur_token->type != IDENTIFIER)
      return ERR_SYNTAX;

    // Make identifier node
    ASTNode idtf;
    err = make_child_node(func_def, &idtf, NODE_IDENTIFIER, DT_UNDEF,
                          cur_token->value);
    IF_ERROR_RETURN(err);

    // Continue parsing
    // (
    cur_token = get_next_token();
    if (cur_token->type != OPEN_BRACKET)
      return ERR_SYNTAX;
    cur_token = NULL; // Token was parsed, "consume" it

    // <param_list>
    err = parse_param_list(func_def);
    IF_ERROR_RETURN(err);
    // Parsing function might have "consumed" token
    if (cur_token == NULL) {
      cur_token = get_next_token();
    }

    // )
    if (cur_token->type != CLOSE_BRACKET)
      return ERR_SYNTAX;
    cur_token = NULL; // Token was parsed, "consume" it

    // <data_type_opt>
    err = parse_data_type_opt(idtf, func_def);
    IF_ERROR_RETURN(err);
    // Parsing function might have "consumed" token
    if (cur_token == NULL) {
      cur_token = get_next_token();
    }

    // =
    if (cur_token->type != ASSIGNMENT)
      return ERR_SYNTAX;
    cur_token = NULL; // Token was parsed, "consume" it

    // <fun_body>
    err = parse_fun_body(func_def);
    IF_ERROR_RETURN(err);

    return SUCCESS;
  }
  default:
    break;
  }

  return ERR_SYNTAX;
}

static ErrCode parse_param_list(ASTNode func_def) {
  // TODO

  cur_token = NULL;
  return SUCCESS;
}

static ErrCode parse_data_type_opt(ASTNode idtf, ASTNode opt) {

  // if (cur_token == NULL)
  //   cur_token = get_next_token();

  // Identifier is required, opt is not
  assert(idtf != NULL);

  // TODO
  cur_token = NULL;

  return SUCCESS;
}

static ErrCode parse_fun_body(ASTNode func_def) {

  // TODO
  cur_token = NULL;
  return SUCCESS;
}
