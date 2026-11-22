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
  UPDATE_CUR_TOKEN();

  // Match rule
  switch (cur_token->type) {
  case KW_IMPORT: {
    // Token was parsed
    cur_token = NULL;

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
  UPDATE_CUR_TOKEN();

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

    // <fun_def_list> -> \NL <fun_def_any> <fun_def_list>
    // Token was parsed
    cur_token = NULL;
    err = parse_fun_def_any(ast_root);
    RETURN_ON_ERROR(err, "inside fun def list ");
    err = parse_fun_def_list(ast_root);
    RETURN_ON_ERROR(err, "further inside fun def list");

    return SUCCESS;
  }
  case END_OF_FILE: {

    //<fun_def_list> -> epsilon
    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing fun def list");
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
  //  Match rule
  switch (cur_token->type) {
  case AT_MAIN: {

    // <fun_def_any> -> @main <fun_def>
    // Token was parsed
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
  UPDATE_CUR_TOKEN();

  // 7 <fun_def> -> def IDENTIFIER (<param_list>) <data_type_opt> = <fun_body>
  //  Match rule
  switch (cur_token->type) {
  case KW_DEF: {

    // <fun_def> -> def IDENTIFIER (<param_list>) <data_type_opt> = <fun_body>
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
  //  Match rule
  switch (cur_token->type) {
  case CLOSE_BRACKET: {

    // <param_list> -> epsilon
    return SUCCESS;
  }
  case IDENTIFIER: {

    // <param_list> -> <param> <param_sep>
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
}

static ErrCode parse_param(ASTNode param_list) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 12 <param> -> IDENTIFIER : <data_type>
  //  Match rule
  switch (cur_token->type) {
  case IDENTIFIER: {

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
}

static ErrCode parse_param_sep(ASTNode param_list) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 10 <param_sep> -> epsilon
  // 11 <param_sep> -> , <param> <param_sep>
  //  Match rule
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
}

static ErrCode parse_data_type_opt(ASTNode idtf, ASTNode extra_node) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Identifier is required, opt is not
  assert(idtf != NULL);

  // 13 <data_type_opt> -> epsilon
  // 14 <data_type_opt> -> : <data_type>
  //  Match rule
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
}

static ErrCode parse_data_type(ASTNode idtf) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();
  assert(idtf != NULL);

  // 15 <data_type> -> Int
  // 16 <data_type> -> Double
  // 17 <data_type> -> String
  // 18 <data_type> -> Unit
  //  Match rule
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
}

static ErrCode parse_fun_body(ASTNode func_def) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 19 <fun_body> -> <expr_or_call>
  // 20 <fun_body> -> <block>
  //  Match rule
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
}

static ErrCode parse_block(ASTNode parent) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // Make block AST node
  ASTNode block;
  err = make_child_node(parent, &block, NODE_BLOCK, DT_UNDEF, NULL);
  RETURN_ON_ERROR(err, "inside block");

  // 21 <block> -> \NL INDENT <stmts> DEDENT
  //  Match rule
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
    cur_token = NULL;

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing block");
}

static ErrCode parse_stmts(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 22 <stmts> -> <stmt> <stmt_sep>
  //  Match rule
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
  //  Match rule
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
    err = parse_if_stmt(block);
    RETURN_ON_ERROR(err, "inside stmt, if");

    return SUCCESS;
  }
  case KW_WHILE: {

    // 31 <stmt> -> <while_loop>
    err = parse_while_loop(block);
    RETURN_ON_ERROR(err, "inside stmt, while");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing statement");
}

static ErrCode parse_decision(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // DECIDE IF THIS IS A FUNCTION CALL OR EXPRESSION
  // TODO DOCUMENT!!!
  // 103 <decision> -> <expr_or_call>
  // 104 <decision> -> <assignment>
  //  Match rule
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
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing decision");
}

static ErrCode parse_expr_or_call(ASTNode parent) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // DECIDE IF THIS IS A FUNCTION CALL OR EXPRESSION
  // TODO DOCUMENT!!!
  // 101 <expr_or_call> -> <fun_call>
  // 102 <expr_or_call> -> <expr>
  //  Match rule
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
      err = parse_expr(parent);
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
    err = parse_expr(parent);
    RETURN_ON_ERROR(err, "inside expr or call, expression");

    return SUCCESS;
  }
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing expr or call");
}

static ErrCode parse_stmt_sep(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 23 <stmt_sep> -> ; <stmt_next> <stmt_sep>
  // 24 <stmt_sep> -> \NL <stmt_next>
  //  Match rule
  switch (cur_token->type) {
  case SEMICOLON: {

    // 23 <stmt_sep> -> ; <stmt_next> <stmt_sep>
    // ; was parsed
    cur_token = NULL;

    // <stmt_next>
    err = parse_stmt_next(block);
    RETURN_ON_ERROR(err, "inside next statement");

    // <stmt_sep>
    err = parse_stmt_sep(block);
    RETURN_ON_ERROR(err, "inside next statement separator");

    return SUCCESS;
  }
  case NEW_LINE: {
    
    // 24 <stmt_sep> -> \NL <stmt_next>
    // \NL was parsed
    cur_token = NULL;

    // <stmt_next>
    err = parse_stmt_next(block);
    RETURN_ON_ERROR(err, "inside next statement");

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing statement separator");
}

static ErrCode parse_stmt_next(ASTNode block) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  // 25 <stmt_next> -> epsilon
  // 26 <stmt_next> -> <stmts>
  //  Match rule
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
}

static ErrCode parse_template(ASTNode template_node) {

  ErrCode err = SUCCESS;
  UPDATE_CUR_TOKEN();

  RETURN_ON_ERROR(ERR_SYNTAX, "replace template message!");
}