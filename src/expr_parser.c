// Project: prekladac-ifj26
// File: expr_parser.c
// Author: Martin Vaško (xvaskom00)

// Implementation of parser functions, which parse only expressions using
// precedence syntax analysis = PSA (using precedence table)

#include "expr_parser.h"
#include "expr_parser_priv.h"

#include "ast_module.h"
#include "error_codes.h"
#include "lexer.h"

#include <assert.h>
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

// Enum for deciding which function to use
// Actions represent the < > = symbols inside PSA table
typedef enum psa_actions {
  S,         // ACT_SHIFT, <
  R,         // ACT_REDUCE, >
  P,         // ACT_PUSH_ONLY, =
  I,         // ACT_INVALID, empty slot
  ACT_WRONG, // For detecting my programming errors
} PSAAction;

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

static void PSA_stack_destroy(PSAStack stack);

static PSAVal tokentype_to_psaval(TokenType token_type) {

  // Find equivalent PSAVal
  switch (token_type) {
  case IDENTIFIER: {
    return PS_IDENTIFIER;
  }
  case VAL_INT: {
    return PS_INT;
  }
  case VAL_DOUBLE: {
    return PS_DOUBLE;
  }
  case VAL_STRING: {
    return PS_STRING;
  }
  // No Unit token
  case OP_MULT: {
    return PS_MULT;
  }
  case OP_DIV: {
    return PS_DIV;
  }
  case OP_PLUS: {
    return PS_PLUS;
  }
  case OP_MINUS: {
    return PS_MINUS;
  }

  case OP_LESS: {
    return PS_LESS;
  }
  case OP_LESS_EQ: {
    return PS_LESS_EQ;
  }
  case OP_GREATER: {
    return PS_GREATER;
  }
  case OP_GREATER_EQ: {
    return PS_GREATER_EQ;
  }
  case OP_EQ: {
    return PS_EQ;
  }
  case OP_NOT_EQ: {
    return PS_NOT_EQ;
  }
  case OPEN_BRACKET: {
    return PS_OPEN_BRACKET;
  }
  case CLOSE_BRACKET: {
    return PS_CLOSE_BRACKET;
  }

  // Valid expressions end only with semicolon or new line
  case SEMICOLON:
  case NEW_LINE: {
    return PS_END;
  }

  // Other token type
  default: {
    return PS_UNKNOWN;
  }
  }
}

// PSA Table equivalent to the one in documentation
// The only difference is that $ on stack and $ on input shouldn't be checked,
// so there is not OK, but ACT_WRONG
// clang-format off
static const PSAAction psa_table[7][7] = {
    {R, R, R, S, R, S, R},
    {S, R, R, S, R, S, R},
    {S, S, I, S, R, S, R},
    {S, S, S, S, P, S, I},
    {R, R, R, I, R, I, R}, 
    {R, R, R, I, R, I, R},
    {S, S, S, S, I, S, ACT_WRONG},
};
// clang-format on

static int get_PSA_table_idx(PSAVal val) {

  // Match grouped values from table
  switch (val) {
  case PS_MULT:
  case PS_DIV: {

    // * has the same index as *, /
    return 0;
  }
  case PS_PLUS:
  case PS_MINUS: {

    // + has the same index as +, -
    return 1;
  }
  case PS_LESS:
  case PS_LESS_EQ:
  case PS_GREATER:
  case PS_GREATER_EQ:
  case PS_EQ:
  case PS_NOT_EQ: {

    // < has the same index as <, >, <=, >=, ==, !=
    return 2;
  }
  case PS_OPEN_BRACKET: {

    // ( has unique index
    return 3;
  }
  case PS_CLOSE_BRACKET: {

    // ) has unique index
    return 4;
  }
  case PS_IDENTIFIER:
  case PS_INT:
  case PS_DOUBLE:
  case PS_STRING:
  case PS_UNIT: {

    // i (= IDENTIFIER) has the same index as i and all literals
    return 5;
  }
  case PS_BEGIN:
  case PS_END: {
    // Both $ have the same index
    return 6;
  }

  default: {
    fprintf(stderr, "Bad get_PSA_table_idx argument!!!\n");
    return -1;
  }
  }
}

static PSAAction PSA_lookup_table(PSAStack stack, PSAVal input) {

  // We need to compare first terminal from top of stack
  PSAVal term;
  PSAStack search = stack;
  while (search->val == PS_EXPR) {
    search = search->previous;

    // We should find at least PS_BEGIN, if we don't, the stack is invalid
    assert(search != NULL);
  }

  term = search->val;
  // Row index
  int term_idx = get_PSA_table_idx(term);
  // Column index
  int input_idx = get_PSA_table_idx(input);

  // Sanity check
  assert(term_idx != -1 && input_idx != -1);

  // Get action from table
  return psa_table[term_idx][input_idx];
}

static ErrCode PSA_shift(PSAStack stack, PSAVal input);
static ErrCode PSA_reduce(PSAStack stack);
static ErrCode PSA_push(PSAStack stack, PSAVal input);

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

ErrCode parse_expr(ASTNode parent, Token **cur_token_ptr) {

  // Copy current token into local static variable
  cur_token = *cur_token_ptr;
  ErrCode err = SUCCESS;

  // Prepare PSA stack
  PSAStack stack;
  err = PSA_stack_init(&stack);
  RETURN_ON_ERROR(err, "inside expression");

  while (PSA_finished(stack) == false) {

    // Get next token if cur_token is NULL
    UPDATE_CUR_TOKEN();

    // Turn token into PSAVal
    PSAVal input = tokentype_to_psaval(cur_token->type);
    if (input == PS_UNKNOWN) {
      PSA_stack_destroy(stack);
      fprintf(stderr, "failed parsing expression, wrong token in expression\n");
      return ERR_SYNTAX;
    }

    // Figure out what to do
    PSAAction act = PSA_lookup_table(stack, input);
    // Sanity check
    assert(act != ACT_WRONG);

    // Match action
    switch (act) {
    case S: {

      PSA_shift(stack, input);
      break;
    }

    case R: {

      PSA_reduce(stack);
      break;
    }

    case P: {

      PSA_push(stack, input);
      break;
    }
    case I: {

      PSA_stack_destroy(stack);
      fprintf(stderr, "failed parsing expression, malformed expression\n");
      return ERR_SYNTAX;
    }
    default: {

      PSA_stack_destroy(stack);
      fprintf(stderr, "Bad parse_expr!!!\n");
      return ERR_INTERNAL;
    }
    }
  } // while parsing not done

  // Cleanup
  PSA_stack_destroy(stack);

  // Update foreign cur_token before returning
  *cur_token_ptr = cur_token;

  return SUCCESS;
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