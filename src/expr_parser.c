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

// cur_token and UPDATE_CUR_TOKEN() were copied from parser.c, for info check
// comments there. I copied it here so I can work with current token in the same
// way as in parser.c.
static Token *cur_token;

#define UPDATE_CUR_TOKEN()                                                     \
  do {                                                                         \
    if (cur_token == NULL) {                                                   \
      cur_token = get_next_token();                                            \
    }                                                                          \
  } while (0)

// Possible values of PSA nodes
// Most of them represent an equivalent TokenType, but not all
typedef enum psa_values {

  PS_BEGIN,      // In table: $
  PS_RULE_START, // In table: <
  PS_EXPR,       // In grammar: E
  // Symbols from table > and = don't need to be saved

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

  // Initialization
  new_stack->previous = NULL;
  new_stack->val = PS_BEGIN;

  // Pass pointer to variable outside
  *stack = new_stack;
  return SUCCESS;
}

static ErrCode PSA_stack_destroy(PSAStack stack);

static ErrCode PSA_shift(PSAStack stack, Token *cur_token);
static ErrCode PSA_reduce(PSAStack stack);

static bool PSA_finished(PSAStack stack) {

  assert(stack != NULL);

  // 1st condition: stack only has: $E
  if (stack->val == PS_EXPR && stack->previous != NULL &&
      stack->previous->val == PS_BEGIN) {

    // 2nd condition: current token is: $
    // That means it's the end of expression, which only happens when
    // current token is new line or semicolon
    if (cur_token->type == NEW_LINE || cur_token->type == SEMICOLON) {
      // PSA is done!
      return true;
    }
  }

  // If conditions aren't met, PSA isn't finished
  return false;
}

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

  // Copy current token into local static variable
  cur_token = *cur_token_ptr;
  UPDATE_CUR_TOKEN();

  ErrCode err = SUCCESS;

  // Prepare PSA stack
  PSAStack stack;
  err = PSA_stack_init(&stack);
  RETURN_ON_ERROR(err, "inside expression");

  while (PSA_finished(stack) == false) {

    // TODO
  }

  // Update foreign cur_token before returning
  *cur_token_ptr = cur_token;

  return SUCCESS;
}