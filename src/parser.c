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
  RETURN_ON_ERROR(err, "while initializing AST child node");

  // Dereference
  // It's ok because type ASTNode is a pointer to structure on heap
  ASTNode child = *ptr_uninit;

  // Add new node into tree
  err = AST_node_add_child(parent, child);
  if (err != SUCCESS) {
    // If it fails, we need to get rid of the child
    AST_destroy_tree(child);
    RETURN_ON_ERROR(err, "while adding AST child to parent");
  }

  return SUCCESS;
}

static ErrCode parse_program(ASTNode ast_root) {

  // AST root node is initialized already

  cur_token = get_next_token();
  assert(cur_token != NULL); // NULL means program wasn't tokenized

  ErrCode err = SUCCESS;

  // Match rule
  switch (cur_token->type) {
  case KW_IMPORT: {

    // <program> -> <prolog> <fun_def_list>
    // <prolog> -> import ifj26.*
    err = parse_prolog();
    RETURN_ON_ERROR(err, "inside program");

    // <fun_def_list>
    err = parse_fun_def_list(ast_root);
    RETURN_ON_ERROR(err, "inside program");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "expected \"import\"");
}

static ErrCode parse_prolog(void) {

  ErrCode err = SUCCESS;
  if (cur_token == NULL)
    cur_token = get_next_token();

  // Match rule
  switch (cur_token->type) {
  case KW_IMPORT: {

    // <prolog> -> import ifj26.*
    // expected ifj26.* in tokens is identifier, dot, op_mult
    // we do a little bit of workaround here
    cur_token = get_next_token();
    bool success = (cur_token->type == IDENTIFIER &&
                    strcmp(cur_token->value, "ifj26") == 0);
    if (success == false)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside prolog, expected \"ifj26\"");

    cur_token = get_next_token();
    if (cur_token->type != DOT)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside prolog, expected \".\"");

    cur_token = get_next_token();
    if (cur_token->type != OP_MULT)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside prolog, expected \"*\"");

    // Token was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing prolog");
}

static ErrCode parse_fun_def_list(ASTNode ast_root) {

  ErrCode err = SUCCESS;
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
    err = parse_fun_def_any(ast_root);
    RETURN_ON_ERROR(err, "inside fun def list ");
    err = parse_fun_def_list(ast_root);
    RETURN_ON_ERROR(err, "further inside fun def list");

    return SUCCESS;
  }
  case END_OF_FILE: {

    //<fun_def_list> -> epsilon
    // No need to cur_token = NULL; here
    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing fun def list");
}

static ErrCode parse_fun_def_any(ASTNode ast_root) {

  ErrCode err = SUCCESS;
  if (cur_token == NULL)
    cur_token = get_next_token();

  // Prepare unfinished node for function definition
  ASTNode func_def;
  err = make_child_node(ast_root, &func_def, NODE_FUNC_DEF, DT_UNDEF, NULL);
  RETURN_ON_ERROR(err, "inside fun def any");

  // 5 <fun_def_any> -> @main <fun_def>
  // 6 <fun_def_any> -> <fun_def>
  //  Match rule
  switch (cur_token->type) {
  case AT_MAIN: {

    // <fun_def_any> -> @main <fun_def>
    cur_token = NULL;                         // Token was parsed
    func_def->node_type = NODE_MAIN_FUNC_DEF; // It is @main!
    err = parse_fun_def(func_def);
    RETURN_ON_ERROR(err, "inside @main func def");

    return SUCCESS;
  }
  case KW_DEF: {

    // <fun_def_any> -> <fun_def>
    err = parse_fun_def(func_def);
    RETURN_ON_ERROR(err, "inside func def");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing fun def any");
}

static ErrCode parse_fun_def(ASTNode func_def) {

  ErrCode err = SUCCESS;
  if (cur_token == NULL)
    cur_token = get_next_token();

  //  Match rule
  switch (cur_token->type) {
  case KW_DEF: {

    // <fun_def> -> def IDENTIFIER (<param_list>) <data_type_opt> = <fun_body>
    cur_token = get_next_token();
    if (cur_token->type != IDENTIFIER)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside fun def, expected identifier");

    // Make identifier node
    ASTNode idtf;
    err = make_child_node(func_def, &idtf, NODE_IDENTIFIER, DT_UNDEF,
                          cur_token->value);
    RETURN_ON_ERROR(err, "inside fun def");

    // Continue parsing
    // (
    cur_token = get_next_token();
    if (cur_token->type != OPEN_BRACKET)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside fun def, expected \"(\"");
    cur_token = NULL; // Token was parsed, "consume" it

    // <param_list>
    err = parse_param_list(func_def);
    RETURN_ON_ERROR(err, "inside fun def params");
    // Parsing function might have "consumed" token
    if (cur_token == NULL) {
      cur_token = get_next_token();
    }

    // )
    if (cur_token->type != CLOSE_BRACKET)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside fun def, expected \")\"");
    cur_token = NULL; // Token was parsed, "consume" it

    // <data_type_opt>
    err = parse_data_type_opt(idtf, func_def);
    RETURN_ON_ERROR(err, "inside fun def data type");
    // Parsing function might have "consumed" token
    if (cur_token == NULL) {
      cur_token = get_next_token();
    }

    // =
    if (cur_token->type != ASSIGNMENT)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside fun def, expected \"=\"");
    cur_token = NULL; // Token was parsed, "consume" it

    // <fun_body>
    err = parse_fun_body(func_def);
    RETURN_ON_ERROR(err, "inside fun def body");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing fun def");
}

static ErrCode parse_param_list(ASTNode func_def) {

  ErrCode err = SUCCESS;
  // TODO

  //cur_token = NULL;
  return SUCCESS;
}

static ErrCode parse_data_type_opt(ASTNode idtf, ASTNode opt) {

  ErrCode err = SUCCESS;

  // if (cur_token == NULL)
  //   cur_token = get_next_token();

  // Identifier is required, opt is not
  assert(idtf != NULL);

  // TODO
  //cur_token = NULL;

  return SUCCESS;
}

static ErrCode parse_fun_body(ASTNode func_def) {

  ErrCode err = SUCCESS;

  // TODO
  //cur_token = NULL;
  return SUCCESS;
}
