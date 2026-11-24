// Project: prekladac-ifj26
// File: expr_parser.c
// Author: Martin Vaško (xvaskom00)

// Implementation of parser functions, which parse only expressions using
// precedence syntax analysis = PSA (using precedence table)

#include "expr_parser.h"

#include "ast_module.h"
#include "error_codes.h"
#include "lexer.h"

#include <stdlib.h>

// Possible values of PSA nodes
// Most of them represent an equivalent TokenType, but not all
typedef enum psa_values {

  PS_BEGIN,
  PS_RULE_START,
  PS_EXPR,

  PS_IDENTIFIER,
  PS_INT,
  PS_DOUBLE,
  PS_STRING,

  PS_UNIT, // () will be converted into Unit

  PS_MULT,
  PS_DIV,
  PS_PLUS,
  PS_MINUS,

  PS_LESS,
  PS_LESS_EQ,
  PS_GREATER,
  PS_GREATER_EQ,
  PS_EQ, // ==
  PS_NOT_EQ,

  PS_OPEN_BRACKET,
  PS_CLOSE_BRACKET,

} PSAVal;

// Stack made from linked nodes
typedef struct psa_stack_st {

  // Node no stack below this one
  struct psa_stack_st *previous;

  PSAVal val;

} *PSAStack;

/**
 * @brief Initializes stack for precedence syntax analysis, 
 * and inserts value `PS_BEGIN`.
 * @param stack Pointer to declared uninitialized `PSAStack` variable.
 * @return `SUCCESS` if it succeeds, otherwise `ERR_INTERNAL`.
 */
static ErrCode PSA_stack_init(PSAStack *stack) {

  // Allocate memory for stack node
  PSAStack new_stack = malloc(sizeof(struct psa_stack_st));
  if (new_stack == NULL) {
    RETURN_ON_ERROR(ERR_INTERNAL, "memory allocation for PSA stack failed");
  }

  // Initialize
  new_stack->previous = NULL;
  new_stack->val = PS_BEGIN;

  // Pass pointer to variable outside
  *stack = new_stack;
  return SUCCESS;
}

static ErrCode PSA_stack_destroy(PSAStack *stack);

static ErrCode PSA_shift();
static ErrCode PSA_reduce();

// ErrCode PSA_;

ErrCode parse_cond(ASTNode parent, Token **cur_token_ptr) {

  return SUCCESS;

  ErrCode err = SUCCESS;
  // UPDATE_CUR_TOKEN();

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
  // UPDATE_CUR_TOKEN();

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