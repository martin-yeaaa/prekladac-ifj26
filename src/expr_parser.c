// Project: prekladac-ifj26
// File: expr_parser.c
// Author: Martin Vaško (xvaskom00)

// Implementation of parser functions, which parse only expressions using
// precedence syntax analysis (using precedence table)

#include "expr_parser.h"

#include "ast_module.h"
#include "error_codes.h"
#include "lexer.h"

ErrCode parse_cond(ASTNode parent, Token **cur_token_ptr) {

  return SUCCESS;

  ErrCode err = SUCCESS;
  //UPDATE_CUR_TOKEN();

  // ruleeeeees
  // Match rule
  switch ((*cur_token_ptr)->type) {
  case 67: {

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing condition");
  return ERR_SYNTAX;
}

ErrCode parse_expr(ASTNode parent, Token **cur_token_ptr) {

  return SUCCESS;

  ErrCode err = SUCCESS;
  //UPDATE_CUR_TOKEN();

  // ruleeeeees
  // Match rule
  switch ((*cur_token_ptr)->type) {
  case 67: {

    return SUCCESS;
  }
  default:
    break;
  }

  RETURN_ON_ERROR(ERR_SYNTAX, "failed parsing expression");
  return ERR_SYNTAX;
}