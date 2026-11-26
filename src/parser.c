// Project: prekladac-ifj26
// File: parser.c
// Author: Martin Vaško (xvaskom00)

// Implementation of parser functions, which do syntax analysis based on
// an LL grammar using the recursive descent method.
// This excludes parsing of expressions and conditions.

#include "parser.h"
#include "parser_priv.h"

#include "expr_parser.h" // Parsing expressions using different method

#include "ast_module.h"
#include "codegen.h"
#include "error_codes.h"
#include "lexer.h"
#include "semantics.h"
#include "symtable.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Global variable that holds current token.
// It's easier to keep track of it this way, otherwise we would need to pass
// a pointer to pointer to token in every parsing function.
static Token *cur_token;

// This macro will update global cur_token as needed
#define UPDATE_CUR_TOKEN()                                                     \
  do {                                                                         \
    if (cur_token == NULL) {                                                   \
      cur_token = get_next_token();                                            \
    }                                                                          \
  } while (0)

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

  // AST is done, output IFJcode26
  generate_code(program_tree);

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

    // 1 <program> -> <prolog> <fun_def_list_nl>
    // <prolog>
    err = parse_prolog();
    RETURN_ON_ERROR(err, "inside program");

    // <fun_def_list_nl>
    err = parse_fun_def_list_nl(ast_root);
    RETURN_ON_ERROR(err, "inside program");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "expected \"import\"");
  return ERR_SYNTAX;
}

static ErrCode parse_prolog(void) {

  UPDATE_CUR_TOKEN();

  // Match rule
  switch (cur_token->type) {
  case KW_IMPORT: {
    // Token was parsed
    cur_token = NULL;

    // 2 <prolog> -> import ifj26.*
    // expected ifj26.* in tokens is identifier, dot, op_mult
    // we do a little bit of workaround here
    cur_token = get_next_token();
    bool success = (cur_token->type == IDENTIFIER &&
                    strcmp(cur_token->value, "ifj26") == 0);
    if (success == false)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside prolog, expected \"ifj26\"");

    // ifj26 parsed, replace with next token
    cur_token = get_next_token();
    if (cur_token->type != DOT)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside prolog, expected \".\"");

    // "." parsed, replace with next token
    cur_token = get_next_token();
    if (cur_token->type != OP_MULT)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside prolog, expected \"*\"");

    // "*" was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing prolog");
  return ERR_SYNTAX;
}

static ErrCode parse_fun_def_list_nl(ASTNode ast_root) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 300 <fun_def_list_nl> -> \NL <fun_def_list> EOF
  // Match rule
  switch (cur_token->type) {
  case NEW_LINE: {

    // \NL was parsed
    cur_token = NULL;

    // <fun_def_list>
    err = parse_fun_def_list(ast_root);
    if (err != SUCCESS)
      return err; // No error message clutter

    UPDATE_CUR_TOKEN();
    // EOF
    if (cur_token->type != END_OF_FILE)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside fun def list nl, expected \"EOF\"");
    // EOF was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX,
                  "failed parsing fun def list nl, expected NEW LINE");
  return ERR_SYNTAX;
}

static ErrCode parse_fun_def_list(ASTNode ast_root) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 3 <fun_def_list> -> epsilon
  // 4 <fun_def_list> -> <fun_def_any> <fun_def_list_nl>
  // Match rule
  switch (cur_token->type) {
  case END_OF_FILE: {

    // 3 <fun_def_list> -> epsilon
    return SUCCESS;
  }
  case AT_MAIN:
  case KW_DEF: {

    // 4 <fun_def_list> -> <fun_def_any> <fun_def_list_nl>
    // <fun_def_any>
    err = parse_fun_def_any(ast_root);
    RETURN_ON_ERROR(err, "inside fun def list");

    // <fun_def_list_nl>
    err = parse_fun_def_list_nl(ast_root);
    RETURN_ON_ERROR(err, "further inside fun def list");

    return SUCCESS;
  }
  default: {
    break;
  }
  }

  RETURN_ON_ERROR(ERR_SYNTAX,
                  "failed parsing fun def list, expected EOF, @main or def");
  return ERR_SYNTAX;
}

static ErrCode parse_fun_def_any(ASTNode ast_root) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Prepare unfinished node for function definition
  ASTNode func_def;
  err = make_child_node(ast_root, &func_def, NODE_FUNC_DEF, DT_UNDEF, NULL);
  RETURN_ON_ERROR(err, "inside fun def any");

  // 5 <fun_def_any> -> @main <fun_def>
  // 6 <fun_def_any> -> <fun_def>
  // Match rule
  switch (cur_token->type) {
  case AT_MAIN: {

    // 5 <fun_def_any> -> @main <fun_def>
    // Token @main was parsed
    cur_token = NULL;
    func_def->node_type = NODE_MAIN_FUNC_DEF; // It is @main!

    // Lexer gives new line here, even though it's not required!
    // Now I need to fix this :(
    cur_token = get_next_token();
    if (cur_token->type == NEW_LINE) {
      // Simply skip to next token
      cur_token = get_next_token();
    }

    // <fun_def>
    err = parse_fun_def(func_def);
    RETURN_ON_ERROR(err, "inside @main func def");

    return SUCCESS;
  }
  case KW_DEF: {

    // 6 <fun_def_any> -> <fun_def>
    // <fun_def>
    err = parse_fun_def(func_def);
    RETURN_ON_ERROR(err, "inside func def");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX,
                  "failed parsing fun def any, expected @main or def");
  return ERR_SYNTAX;
}

static ErrCode parse_fun_def(ASTNode func_def) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 7 <fun_def> -> def IDENTIFIER (<param_list>) <data_type_opt> = <fun_body>
  // Match rule
  switch (cur_token->type) {
  case KW_DEF: {

    // 7 <fun_def> -> def IDENTIFIER (<param_list>) <data_type_opt> = <fun_body>
    // def was parsed, replace with next
    cur_token = get_next_token();
    if (cur_token->type != IDENTIFIER)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside fun def, expected identifier");

    // Make identifier node
    ASTNode idtf;
    err = make_child_node(func_def, &idtf, NODE_IDENTIFIER, DT_UNDEF,
                          cur_token->value);
    RETURN_ON_ERROR(err, "inside fun def");

    // IDENTIFIER was parsed, replace with next
    cur_token = get_next_token();
    // (
    if (cur_token->type != OPEN_BRACKET)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside fun def, expected \"(\"");
    // ( was parsed
    cur_token = NULL;

    // <param_list>
    // AST node for param list is made inside parse_param_list
    err = parse_param_list(func_def);
    RETURN_ON_ERROR(err, "inside fun def params");

    // Parsing function might have "consumed" token
    UPDATE_CUR_TOKEN();
    if (cur_token->type != CLOSE_BRACKET)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside fun def, expected \")\"");
    // ) was parsed
    cur_token = NULL;

    // <data_type_opt>
    err = parse_data_type_opt(idtf, func_def);
    RETURN_ON_ERROR(err, "inside fun def data type opt");

    // Parsing function might have "consumed" token
    UPDATE_CUR_TOKEN();
    // =
    if (cur_token->type != ASSIGNMENT)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside fun def, expected \"=\"");
    // = was parsed
    cur_token = NULL;

    // <fun_body>
    err = parse_fun_body(func_def);
    RETURN_ON_ERROR(err, "inside fun def body");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing fun def");
  return ERR_SYNTAX;
}

static ErrCode parse_param_list(ASTNode func_def) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // parse_param_list should be called only once per func def
  ASTNode param_list;
  err = make_child_node(func_def, &param_list, NODE_PARAM_LIST, DT_UNDEF, NULL);
  RETURN_ON_ERROR(err, "inside param list");

  // 8 <param_list> -> epsilon
  // 9 <param_list> -> <param> <param_sep>
  // Match rule
  switch (cur_token->type) {
  case CLOSE_BRACKET: {

    // 8 <param_list> -> epsilon
    return SUCCESS;
  }
  case IDENTIFIER: {

    // 9 <param_list> -> <param> <param_sep>
    // <param>
    err = parse_param(param_list);
    RETURN_ON_ERROR(err, "inside param list, param");

    // <param_sep>
    err = parse_param_sep(param_list);
    RETURN_ON_ERROR(err, "inside param list, param separator");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing param list");
  return ERR_SYNTAX;
}

static ErrCode parse_param(ASTNode param_list) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 12 <param> -> IDENTIFIER : <data_type>
  // Match rule
  switch (cur_token->type) {
  case IDENTIFIER: {

    // 12 <param> -> IDENTIFIER : <data_type>
    // Make parameter
    ASTNode param;
    err = make_child_node(param_list, &param, NODE_IDENTIFIER, DT_UNDEF,
                          cur_token->value);
    RETURN_ON_ERROR(err, "inside param");

    // IDENTIFIER was parsed, replace with next
    cur_token = get_next_token();
    // :
    if (cur_token->type != COLON)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside param, expected \":\"");
    // : was parsed
    cur_token = NULL;

    // <data_type>
    err = parse_data_type(param);
    RETURN_ON_ERROR(err, "inside param, data type");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing param");
  return ERR_SYNTAX;
}

static ErrCode parse_param_sep(ASTNode param_list) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 10 <param_sep> -> epsilon
  // 11 <param_sep> -> , <param> <param_sep>
  // Match rule
  switch (cur_token->type) {
  case CLOSE_BRACKET: {

    // 10 <param_sep> -> epsilon
    return SUCCESS;
  }
  case COMMA: {

    // 11 <param_sep> -> , <param> <param_sep>
    // , was parsed
    cur_token = NULL;

    // <param>
    err = parse_param(param_list);
    RETURN_ON_ERROR(err, "inside param list, param");

    // <param_sep>
    err = parse_param_sep(param_list);
    RETURN_ON_ERROR(err, "inside param list, param separator");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing param separator");
  return ERR_SYNTAX;
}

static ErrCode parse_data_type_opt(ASTNode idtf, ASTNode extra_node) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Identifier is required, opt is not
  assert(idtf != NULL);

  // 13 <data_type_opt> -> epsilon
  // 14 <data_type_opt> -> : <data_type>
  // Match rule
  switch (cur_token->type) {
  case ASSIGNMENT: {

    // 13 <data_type_opt> -> epsilon
    return SUCCESS;
  }
  case COLON: {

    // 14 <data_type_opt> -> : <data_type>
    // : was parsed
    cur_token = NULL;

    // <data_type>
    err = parse_data_type(idtf);
    RETURN_ON_ERROR(err, "inside data type opt");

    // AST expects explicitly defined type of FUNCTIONS only to also be in the
    // func_def node!
    if (extra_node != NULL) {
      if (extra_node->node_type == NODE_FUNC_DEF ||
          extra_node->node_type == NODE_MAIN_FUNC_DEF) {
        // Set explicit data type
        extra_node->data_type = idtf->data_type;
      }
    }

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing data type opt");
  return ERR_SYNTAX;
}

static ErrCode parse_data_type(ASTNode idtf) {

  // ErrCode err = SUCCESS; not needed here
  UPDATE_CUR_TOKEN();
  assert(idtf != NULL);

  // 15 <data_type> -> Int
  // 16 <data_type> -> Double
  // 17 <data_type> -> String
  // 18 <data_type> -> Unit
  // Match rule
  switch (cur_token->type) {
  case KW_INT: {
    // 15 <data_type> -> Int
    // Update identifier data type
    idtf->data_type = DT_INTEGER;
    // Int was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  case KW_DOUBLE: {
    // 16 <data_type> -> Double
    // Update identifier data type
    idtf->data_type = DT_DOUBLE;
    // Double was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  case KW_STRING: {
    // 17 <data_type> -> String
    // Update identifier data type
    idtf->data_type = DT_STRING;
    // String was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  case KW_UNIT: {
    // 18 <data_type> -> Unit
    // Update identifier data type
    idtf->data_type = DT_UNIT;
    // Unit was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing data type");
  return ERR_SYNTAX;
}

static ErrCode parse_fun_body(ASTNode func_def) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 19 <fun_body> -> <expr_or_call>
  // 20 <fun_body> -> <block>
  // Match rule
  switch (cur_token->type) {
  case IDENTIFIER:
  case OPEN_BRACKET:
  case OP_MINUS:
  case OP_PLUS:
  case VAL_INT:
  case VAL_DOUBLE:
  case VAL_STRING: {

    // 19 <fun_body> -> <expr_or_call>
    err = parse_expr_or_call(func_def);
    RETURN_ON_ERROR(err, "inside fun body");

    return SUCCESS;
  }
  case NEW_LINE: {

    // 20 <fun_body> -> <block>
    err = parse_block(func_def);
    RETURN_ON_ERROR(err, "inside fun body block");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing fun body");
  return ERR_SYNTAX;
}

static ErrCode parse_block(ASTNode parent) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Make block AST node
  ASTNode block;
  err = make_child_node(parent, &block, NODE_BLOCK, DT_UNDEF, NULL);
  RETURN_ON_ERROR(err, "inside block");

  // 21 <block> -> \NL INDENT <stmts> DEDENT
  // Rule 21 also CREATES an artificial token \NL at the very end!!!
  // Match rule
  switch (cur_token->type) {
  case NEW_LINE: {

    // 21 <block> -> \NL INDENT <stmts> DEDENT
    // \NL was parsed, replace with next
    cur_token = get_next_token();
    if (cur_token->type != INDENT)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside block, expected indent");

    // INDENT was parsed
    cur_token = NULL;

    // <stmts>
    err = parse_stmts(block);
    RETURN_ON_ERROR(err, "inside block");
    // Parsing function might have "consumed" token
    UPDATE_CUR_TOKEN();

    if (cur_token->type != DEDENT)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside block, expected dedent");

    // DEDENT was parsed
    // <stmt_sep> expects a newline as a separator after statement, however that
    // is parsed inside the block. Lexer doesn't generate another fake newline
    // token after DEDENT, so we "generate" it here. Works REALLY well :)
    cur_token->type = NEW_LINE;

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing block");
  return ERR_SYNTAX;
}

static ErrCode parse_stmts(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 22 <stmts> -> <stmt> <stmt_sep>
  // Match rule
  switch (cur_token->type) {
  case IDENTIFIER:
  case OPEN_BRACKET:
  case KW_RETURN:
  case KW_VAL:
  case KW_VAR:
  case OP_MINUS:
  case OP_PLUS:
  case VAL_INT:
  case VAL_DOUBLE:
  case VAL_STRING:
  case KW_IF:
  case KW_WHILE: {

    // 22 <stmts> -> <stmt> <stmt_sep>
    // <stmt>
    err = parse_stmt(block);
    RETURN_ON_ERROR(err, "inside statements");

    // <stmt_sep>
    err = parse_stmt_sep(block);
    RETURN_ON_ERROR(err, "inside statements separator");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing statement");
  return ERR_SYNTAX;
}

static ErrCode parse_stmt(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 27 <stmt> -> <val_def>
  // 28 <stmt> -> <var_def>
  // 29 <stmt> -> return <return_stmt>
  // 30 <stmt> -> <if_stmt>
  // 31 <stmt> -> <while_loop>
  // 100 <stmt> -> <decision>
  // Match rule
  switch (cur_token->type) {
  case IDENTIFIER:
  case OPEN_BRACKET:
  case OP_MINUS:
  case OP_PLUS:
  case VAL_INT:
  case VAL_DOUBLE:
  case VAL_STRING: {

    // 100 <stmt> -> <decision>
    err = parse_decision(block);
    RETURN_ON_ERROR(err, "inside stmt, decision");

    return SUCCESS;
  }
  case KW_RETURN: {

    // 29 <stmt> -> return <return_stmt>
    // return was parsed
    cur_token = NULL;

    err = parse_return_stmt(block);
    RETURN_ON_ERROR(err, "inside stmt, return");

    return SUCCESS;
  }
  case KW_VAL: {

    // 27 <stmt> -> <val_def>
    err = parse_val_def(block);
    RETURN_ON_ERROR(err, "inside stmt, val definition");

    return SUCCESS;
  }
  case KW_VAR: {

    // 28 <stmt> -> <var_def>
    err = parse_var_def(block);
    RETURN_ON_ERROR(err, "inside stmt, var definition");

    return SUCCESS;
  }
  case KW_IF: {

    // 30 <stmt> -> <if_stmt>
    // <if_stmt>
    err = parse_if_stmt(block);
    RETURN_ON_ERROR(err, "inside stmt, if");

    return SUCCESS;
  }
  case KW_WHILE: {

    // 31 <stmt> -> <while_loop>
    // <while_loop>
    err = parse_while_loop(block);
    RETURN_ON_ERROR(err, "inside stmt, while");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing statement");
  return ERR_SYNTAX;
}

static ErrCode parse_decision(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Decide if we are reading expression or function call or assignment
  // 103 <decision> -> <expr_or_call>
  // 104 <decision> -> <assignment>
  // Match rule
  switch (cur_token->type) {
  case IDENTIFIER: {

    // We can't choose another rule right now, we need to look ahead
    if (cur_token->next != NULL && cur_token->next->type == ASSIGNMENT) {
      // Now we can decide
      // 104 <decision> -> <assignment>
      err = parse_assignment(block);
      RETURN_ON_ERROR(err, "inside decision, assignment");

      return SUCCESS;

    } else {
      // 103 <decision> -> <expr_or_call>
      err = parse_expr_or_call(block);
      RETURN_ON_ERROR(err, "inside decision, expr or call");

      return SUCCESS;
    }
  }
  case OPEN_BRACKET:
  case OP_MINUS:
  case OP_PLUS:
  case VAL_INT:
  case VAL_DOUBLE:
  case VAL_STRING: {

    // 103 <decision> -> <expr_or_call>
    err = parse_expr_or_call(block);
    RETURN_ON_ERROR(err, "inside decision, expr or call");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing decision");
  return ERR_SYNTAX;
}

static ErrCode parse_expr_or_call(ASTNode parent) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Decide if we are reading expression or function call
  // 101 <expr_or_call> -> <fun_call>
  // 102 <expr_or_call> -> <expr>
  // Match rule
  switch (cur_token->type) {
  case IDENTIFIER: {

    // We can't choose another rule right now, we need to look ahead
    if (cur_token->next != NULL && cur_token->next->type == OPEN_BRACKET) {
      // Now we can decide
      // 101 <expr_or_call> -> <fun_call>
      err = parse_fun_call(parent);
      RETURN_ON_ERROR(err, "inside expr or call, function call");

      return SUCCESS;

    } else {
      // 102 <expr_or_call> -> <expr>
      err = parse_expr(parent, &cur_token, false);
      RETURN_ON_ERROR(err, "inside expr or call, expression");

      return SUCCESS;
    }
  }
  case OPEN_BRACKET:
  case OP_MINUS:
  case OP_PLUS:
  case VAL_INT:
  case VAL_DOUBLE:
  case VAL_STRING: {

    // 102 <expr_or_call> -> <expr>
    err = parse_expr(parent, &cur_token, false);
    RETURN_ON_ERROR(err, "inside expr or call, expression");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing expr or call");
  return ERR_SYNTAX;
}

static ErrCode parse_stmt_sep(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 23 <stmt_sep> -> ; <after_semicolon>
  // 24 <stmt_sep> -> \NL <stmt_next>
  // Match rule
  switch (cur_token->type) {
  case SEMICOLON: {

    // 23 <stmt_sep> -> ; <after_semicolon>
    // ; was parsed
    cur_token = NULL;

    // <after_semicolon>
    err = parse_after_semicolon(block);
    RETURN_ON_ERROR(err, "inside statement separator, semicolon");

    return SUCCESS;
  }
  case NEW_LINE: {

    // 24 <stmt_sep> -> \NL <stmt_next>
    // \NL was parsed
    cur_token = NULL;

    // <stmt_next>
    err = parse_stmt_next(block);
    RETURN_ON_ERROR(err, "inside statement separator, next");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing statement separator");
  return ERR_SYNTAX;
}

static ErrCode parse_after_semicolon(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 200 <after_semicolon> -> <stmts>
  // 201 <after_semicolon> -> \NL <stmt_next>
  // Match rule
  switch (cur_token->type) {
  case IDENTIFIER:
  case OPEN_BRACKET:
  case KW_RETURN:
  case KW_VAL:
  case KW_VAR:
  case OP_MINUS:
  case OP_PLUS:
  case KW_IF:
  case KW_WHILE:
  case VAL_INT:
  case VAL_DOUBLE:
  case VAL_STRING: {

    // 200 <after_semicolon> -> <stmts>
    // <stmts>
    err = parse_stmts(block);
    RETURN_ON_ERROR(err, "inside after semicolon, stmts");

    return SUCCESS;
  }
  case NEW_LINE: {

    // 201 <after_semicolon> -> \NL <stmt_next>
    // \NL was parser
    cur_token = NULL;

    // <stmt_next>
    err = parse_stmt_next(block);
    RETURN_ON_ERROR(err, "inside after semicolon, new line and stmt");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing after semicolon");
  return ERR_SYNTAX;
}

static ErrCode parse_stmt_next(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 25 <stmt_next> -> epsilon
  // 26 <stmt_next> -> <stmts>
  // Match rule
  switch (cur_token->type) {
  case IDENTIFIER:
  case OPEN_BRACKET:
  case KW_RETURN:
  case KW_VAL:
  case KW_VAR:
  case OP_MINUS:
  case OP_PLUS:
  case KW_IF:
  case KW_WHILE:
  case VAL_INT:
  case VAL_DOUBLE:
  case VAL_STRING: {

    // 26 <stmt_next> -> <stmts>
    // <stmts>
    err = parse_stmts(block);
    RETURN_ON_ERROR(err, "inside statement next");

    return SUCCESS;
  }
  case DEDENT: {

    // 25 <stmt_next> -> epsilon
    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing statement next");
  return ERR_SYNTAX;
}

static ErrCode parse_val_def(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Prepare unfinished node for val definition
  ASTNode val_def;
  err = make_child_node(block, &val_def, NODE_VAL_DEF, DT_UNIT, NULL);
  RETURN_ON_ERROR(err, "inside val definition");

  // 39 <val_def> -> val IDENTIFIER <data_type_opt> = <expr_or_call>
  // Match rule
  switch (cur_token->type) {
  case KW_VAL: {

    // 39 <val_def> -> val IDENTIFIER <data_type_opt> = <expr_or_call>
    // val was parsed, replace with next token
    cur_token = get_next_token();

    if (cur_token->type != IDENTIFIER)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside val def, expected identifier");
    // Make identifier node
    ASTNode idtf;
    err = make_child_node(val_def, &idtf, NODE_IDENTIFIER, DT_UNDEF,
                          cur_token->value);
    RETURN_ON_ERROR(err, "inside val def");

    // IDENTIFIER was parsed
    cur_token = NULL;

    // <data_type_opt>
    err = parse_data_type_opt(idtf, NULL);
    RETURN_ON_ERROR(err, "inside val def, data type opt");
    // Function might have "consumed" token, renew it
    UPDATE_CUR_TOKEN();

    // =
    if (cur_token->type != ASSIGNMENT)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside val def, expected \"=\"");
    // = was parsed
    cur_token = NULL;

    // <expr_or_call>
    err = parse_expr_or_call(val_def);
    RETURN_ON_ERROR(err, "inside val def, expr or call");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing val definition");
  return ERR_SYNTAX;
}

static ErrCode parse_var_def(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Prepare unfinished node for var definition
  ASTNode var_def;
  err = make_child_node(block, &var_def, NODE_VAR_DEF, DT_UNIT, NULL);
  RETURN_ON_ERROR(err, "inside var definition");

  // 40 <var_def> -> var IDENTIFIER <data_type_opt> = <expr_or_call>
  // Match rule
  switch (cur_token->type) {
  case KW_VAR: {

    // 40 <var_def> -> var IDENTIFIER <data_type_opt> = <expr_or_call>
    // var was parsed, replace with next token
    cur_token = get_next_token();

    if (cur_token->type != IDENTIFIER)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside var def, expected identifier");
    // Make identifier node
    ASTNode idtf;
    err = make_child_node(var_def, &idtf, NODE_IDENTIFIER, DT_UNDEF,
                          cur_token->value);
    RETURN_ON_ERROR(err, "inside var def");

    // IDENTIFIER was parsed
    cur_token = NULL;

    // <data_type_opt>
    err = parse_data_type_opt(idtf, NULL);
    RETURN_ON_ERROR(err, "inside var def, data type opt");
    // Function might have "consumed" token, renew it
    UPDATE_CUR_TOKEN();

    // =
    if (cur_token->type != ASSIGNMENT)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside var def, expected \"=\"");
    // = was parsed
    cur_token = NULL;

    // <expr_or_call>
    err = parse_expr_or_call(var_def);
    RETURN_ON_ERROR(err, "inside var def, expr or call");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing var definition");
  return ERR_SYNTAX;
}

static ErrCode parse_assignment(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Prepare unfinished node for assignment
  ASTNode assign;
  err = make_child_node(block, &assign, NODE_ASSIGNMENT, DT_UNIT, NULL);
  RETURN_ON_ERROR(err, "inside assignment");

  // 140 <assignment> -> IDENTIFIER = <expr_or_call>
  // Match rule
  switch (cur_token->type) {
  case IDENTIFIER: {

    // 140 <assignment> -> IDENTIFIER = <expr_or_call>
    // Make identifier node
    ASTNode idtf;
    err = make_child_node(assign, &idtf, NODE_IDENTIFIER, DT_UNDEF,
                          cur_token->value);
    RETURN_ON_ERROR(err, "inside assignment, identifier");

    // IDENTIFIER was parsed, replace with next
    cur_token = get_next_token();

    // =
    if (cur_token->type != ASSIGNMENT)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside assignment, expected \"=\"");
    // = was parsed
    cur_token = NULL;

    err = parse_expr_or_call(assign);
    RETURN_ON_ERROR(err, "inside assignment, expr or call");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing assignment");
  return ERR_SYNTAX;
}

static ErrCode parse_return_stmt(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  ASTNode ret_node;
  err = make_child_node(block, &ret_node, NODE_RETURN, DT_UNDEF, NULL);
  RETURN_ON_ERROR(err, "inside return");

  // 37 <return_stmt> -> epsilon
  // 38 <return_stmt> -> <expr_or_call>
  // Match rule
  switch (cur_token->type) {
  case NEW_LINE: {

    // 37 <return_stmt> -> epsilon
    // Standalone return is equivalent to: return ()
    // We will simulate this

    // AST node chain: return - expression - Unit literal
    ASTNode expr_node;
    err = make_child_node(ret_node, &expr_node, NODE_EXPR, DT_UNIT, NULL);
    RETURN_ON_ERROR(err, "inside standalone return");

    ASTNode unit_lit;
    err = make_child_node(expr_node, &unit_lit, NODE_LITERAL, DT_UNIT, NULL);
    RETURN_ON_ERROR(err, "inside standalone return");

    // Also we can set type of return stmt immediately
    ret_node->data_type = DT_UNIT;

    return SUCCESS;
  }
  case IDENTIFIER:
  case OPEN_BRACKET:
  case OP_MINUS:
  case OP_PLUS:
  case VAL_INT:
  case VAL_DOUBLE:
  case VAL_STRING: {

    // 38 <return_stmt> -> <expr_or_call>

    err = parse_expr_or_call(ret_node);
    RETURN_ON_ERROR(err, "inside return, expr or call");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing return statement");
  return ERR_SYNTAX;
}

static ErrCode parse_if_stmt(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  ASTNode if_stmt;
  err = make_child_node(block, &if_stmt, NODE_IF, DT_UNIT, NULL);
  RETURN_ON_ERROR(err, "inside if statement");

  // 72 <if_stmt> -> if <cond> then <block> \NL else <block>
  // Match rule
  switch (cur_token->type) {
  case KW_IF: {

    // 72 <if_stmt> -> if <cond> then <block> \NL else <block>
    // if was parsed
    cur_token = NULL;

    // <cond> is expression, we just set argument is_cond to true
    err = parse_expr(if_stmt, &cur_token, true);
    RETURN_ON_ERROR(err, "inside if, condition");
    // Function might have "consumed" token, renew it
    UPDATE_CUR_TOKEN();

    // then
    if (cur_token->type != KW_THEN)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside if, expected \"then\"");
    // then was parsed
    cur_token = NULL;

    // <block> - creates new block node
    err = parse_block(if_stmt);
    RETURN_ON_ERROR(err, "inside if, then block");
    // Function might have "consumed" token, renew it
    UPDATE_CUR_TOKEN();

    // <block> creates artificial \NL for convenience in <stmt_sep> but that
    // means we need to take care of it here
    if (cur_token->type != NEW_LINE)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside if, expected fake new line");
    // \NL was parsed, replace with next
    cur_token = get_next_token();

    // else
    if (cur_token->type != KW_ELSE)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside if, expected \"else\"");
    // else was parsed
    cur_token = NULL;

    // <block> - creates new block node
    err = parse_block(if_stmt);
    RETURN_ON_ERROR(err, "inside if, else block");

    return SUCCESS;
  }

  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing if statement");
  return ERR_SYNTAX;
}

static ErrCode parse_while_loop(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  ASTNode while_loop;
  err = make_child_node(block, &while_loop, NODE_WHILE, DT_UNIT, NULL);
  RETURN_ON_ERROR(err, "inside while loop");

  // 73 <while_loop> -> while <cond> do <block>
  // Match rule
  switch (cur_token->type) {
  case KW_WHILE: {

    // 73 <while_loop> -> while <cond> do <block>
    // while was parsed
    cur_token = NULL;

    // <cond> is expression, we just set argument is_cond to true
    err = parse_expr(while_loop, &cur_token, true);
    RETURN_ON_ERROR(err, "inside while, condition");
    // Function might have "consumed" token, renew it
    UPDATE_CUR_TOKEN();

    // do
    if (cur_token->type != KW_DO)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside while, expected \"do\"");
    // do was parsed
    cur_token = NULL;

    // <block> - creates new block node
    err = parse_block(while_loop);
    RETURN_ON_ERROR(err, "inside while block");

    return SUCCESS;
  }

  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing while loop");
  return ERR_SYNTAX;
}

static ErrCode parse_fun_call(ASTNode parent) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Make ast node
  ASTNode func_call;
  err = make_child_node(parent, &func_call, NODE_FUNC_CALL, DT_UNDEF, NULL);
  RETURN_ON_ERROR(err, "inside function call");

  // 120 <fun_call> -> IDENTIFIER(<arg_list>)
  // Match rule
  switch (cur_token->type) {
  case IDENTIFIER: {

    // Make identifier node
    ASTNode idtf;
    err = make_child_node(func_call, &idtf, NODE_IDENTIFIER, DT_UNDEF,
                          cur_token->value);
    RETURN_ON_ERROR(err, "inside function call");

    // IDENTIFIER was parsed, replace with next
    cur_token = get_next_token();

    // (
    if (cur_token->type != OPEN_BRACKET)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside func call, expected \"(\"");
    // ( was parsed
    cur_token = NULL;

    // <arg_list>
    // AST node for arg list is made inside parse_arg_list
    err = parse_arg_list(func_call);
    RETURN_ON_ERROR(err, "inside function call, arg list");
    // cur_token might have been parsed
    UPDATE_CUR_TOKEN();

    // )
    if (cur_token->type != CLOSE_BRACKET)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside func call, expected \")\"");
    // ) was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing function call");
  return ERR_SYNTAX;
}

static ErrCode parse_arg_list(ASTNode func_call) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // parse_arg_list should be called only once per func call
  ASTNode arg_list;
  err = make_child_node(func_call, &arg_list, NODE_ARG_LIST, DT_UNDEF, NULL);
  RETURN_ON_ERROR(err, "inside arg list");

  // 46 <arg_list> -> epsilon
  // 47 <arg_list> -> <arg> <arg_sep>
  // Match rule
  switch (cur_token->type) {
  case CLOSE_BRACKET: {

    // 46 <arg_list> -> epsilon
    return SUCCESS;
  }
  case IDENTIFIER:
  case OPEN_BRACKET:
  case VAL_INT:
  case VAL_DOUBLE:
  case VAL_STRING:
  case OP_PLUS:
  case OP_MINUS: {

    // 47 <arg_list> -> <arg> <arg_sep>
    // <arg>
    err = parse_arg(arg_list);
    RETURN_ON_ERROR(err, "inside arg list, arg");

    // <arg_sep>
    err = parse_arg_sep(arg_list);
    RETURN_ON_ERROR(err, "inside arg list, arg separator");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing argument list");
  return ERR_SYNTAX;
}

static ErrCode parse_arg(ASTNode arg_list) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 50 <arg> -> IDENTIFIER
  // 51 <arg> -> <literal>
  // 52 <arg> -> <unary_op> <literal>
  // Match rule
  switch (cur_token->type) {
  case IDENTIFIER: {

    // 50 <arg> -> IDENTIFIER
    // Make identifier node
    ASTNode idtf;
    err = make_child_node(arg_list, &idtf, NODE_IDENTIFIER, DT_UNDEF,
                          cur_token->value);
    RETURN_ON_ERROR(err, "inside arg, identifier");

    // IDENTIFIER was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  case OPEN_BRACKET:
  case VAL_INT:
  case VAL_DOUBLE:
  case VAL_STRING: {

    // 51 <arg> -> <literal>
    err = parse_literal(arg_list);
    RETURN_ON_ERROR(err, "inside arg, literal");

    return SUCCESS;
  }
  case OP_PLUS:
  case OP_MINUS: {

    // 52 <arg> -> <unary_op> <literal>

    ASTNode un_op;
    // Unary operator is needed outside, semantics module expects unary_op node
    // to have a child literal
    // <unary_op>
    err = parse_unary_op(arg_list, &un_op);
    RETURN_ON_ERROR(err, "inside arg, unary op");

    // <literal>
    err = parse_literal(un_op);
    RETURN_ON_ERROR(err, "inside arg, literal");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing argument");
  return ERR_SYNTAX;
}

static ErrCode parse_arg_sep(ASTNode arg_list) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 48 <arg_sep> -> epsilon
  // 49 <arg_sep> -> , <arg> <arg_sep>
  // Match rule
  switch (cur_token->type) {
  case CLOSE_BRACKET: {

    // 48 <arg_sep> -> epsilon

    return SUCCESS;
  }
  case COMMA: {

    // 49 <arg_sep> -> , <arg> <arg_sep>
    // , was parsed
    cur_token = NULL;

    // <arg>
    err = parse_arg(arg_list);
    RETURN_ON_ERROR(err, "inside arg sep, arg");

    // <arg_sep>
    err = parse_arg_sep(arg_list);
    RETURN_ON_ERROR(err, "inside arg sep, arg separator");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing argument separator");
  return ERR_SYNTAX;
}

static ErrCode parse_literal(ASTNode parent) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Declare literal node
  ASTNode lit;

  // 81 <literal> -> VAL_INT
  // 82 <literal> -> VAL_DOUBLE
  // 83 <literal> -> VAL_STRING
  // 84 <literal> -> ()
  // Match rule
  switch (cur_token->type) {
  case VAL_INT: {

    // 81 <literal> -> VAL_INT
    // Make integer
    err = make_child_node(parent, &lit, NODE_LITERAL, DT_INTEGER,
                          cur_token->value);
    RETURN_ON_ERROR(err, "inside literal, int");
    // Token was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  case VAL_DOUBLE: {

    // 82 <literal> -> VAL_DOUBLE
    // Make double
    err = make_child_node(parent, &lit, NODE_LITERAL, DT_DOUBLE,
                          cur_token->value);
    RETURN_ON_ERROR(err, "inside literal, double");
    // Token was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  case VAL_STRING: {

    // 83 <literal> -> VAL_STRING
    // Make string
    err = make_child_node(parent, &lit, NODE_LITERAL, DT_STRING,
                          cur_token->value);
    RETURN_ON_ERROR(err, "inside literal, string");
    // Token was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  case OPEN_BRACKET: {

    // 84 <literal> -> ()
    // ( was parsed, next token
    cur_token = get_next_token();

    // )
    if (cur_token->type != CLOSE_BRACKET)
      RETURN_ON_ERROR(ERR_SYNTAX, "inside literal, expected \")\"");

    // Make Unit
    err = make_child_node(parent, &lit, NODE_LITERAL, DT_UNIT, NULL);
    RETURN_ON_ERROR(err, "inside literal, unit");

    // ) was parsed
    cur_token = NULL;

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing literal");
  return ERR_SYNTAX;
}

static ErrCode parse_unary_op(ASTNode parent, ASTNode *ptr_to_node) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 66 <unary_op> -> -
  // 67 <unary_op> -> +
  // Match rule
  switch (cur_token->type) {
  case OP_MINUS: {

    // 66 <unary_op> -> -
    // - was parsed
    cur_token = NULL;

    err = make_child_node(parent, ptr_to_node, NODE_UN_MINUS, DT_UNDEF, NULL);
    RETURN_ON_ERROR(err, "inside unary op, minus");

    return SUCCESS;
  }
  case OP_PLUS: {

    // 67 <unary_op> -> +
    // + was parsed
    cur_token = NULL;

    err = make_child_node(parent, ptr_to_node, NODE_UN_PLUS, DT_UNDEF, NULL);
    RETURN_ON_ERROR(err, "inside unary op, plus");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing unary operator");
  return ERR_SYNTAX;
}
