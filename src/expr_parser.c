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

// Enum for deciding which function to use
typedef enum psa_actions {
  ACT_SHIFT,
  ACT_REDUCE,
  ACT_PUSH_ONLY,
  ACT_INVALID,
} PSAAction;

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

  // Used only when TokenType cannot be converted to equivalent PSAVal
  PS_UNKNOWN,

} PSAVal;

// Stack made from linked nodes
typedef struct psa_stack_st {

  // Node no stack below this one
  struct psa_stack_st *previous;
  // Saved AST node, so we can create a tree when reducing
  ASTNode node;
  PSAVal val;

} *PSAStack;


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

  // Other token type
  default: {
    return PS_UNKNOWN;
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
  PSAVal term_group = get_PSA_table_group(term);
  PSAVal input_group = get_PSA_table_group(input);

  // Match rule from table
  switch (term_group) {
  case PS_MULT:
  case PS_DIV: {

    // On stack: *, /
    break;
  }
  case PS_PLUS:
  case PS_MINUS: {

    // On stack: +, -

    break;
  }
  case PS_LESS:
  case PS_LESS_EQ:
  case PS_GREATER:
  case PS_GREATER_EQ:
  case PS_EQ:
  case PS_NOT_EQ: {

    // On stack:  <, >, <=, >=, ==, !=
    break;
  }
  case PS_OPEN_BRACKET: {

    // On stack: (
    break;
  }
  case PS_CLOSE_BRACKET: {

    // On stack: )
    break;
  }
  case PS_IDENTIFIER:
  case PS_INT:
  case PS_DOUBLE:
  case PS_STRING:
  case PS_UNIT: {

    // On stack: i
    break;
  }
  case PS_BEGIN: {

    // On stack: $
    break;
  }
  default: {
    fprintf(stderr, "Bad stack!!!\n");
    return ACT_INVALID;
  }
  }
}

static ErrCode PSA_shift(PSAStack stack);
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
  ErrCode err = SUCCESS;

  // Prepare PSA stack
  PSAStack stack;
  err = PSA_stack_init(&stack);
  RETURN_ON_ERROR(err, "inside expression");

  while (PSA_finished(stack) == false) {

    // Get next token if cur_token is NULL
    UPDATE_CUR_TOKEN();
    PSAVal input = tokentype_to_psaval(cur_token->type);
    if (input == PS_UNKNOWN) {
      PSA_stack_destroy(stack);
      fprintf(stderr, "failed parsing expression (1)\n");
      return ERR_SYNTAX;
    }

    // Figure out what to do
    PSAAction act = PSA_lookup_table(stack, input);

    switch (act) {
    case ACT_SHIFT: {

      //
      PSA_shift(stack);
      break;
    }

    case ACT_REDUCE: {

      PSA_reduce(stack);
      break;
    }

    case ACT_PUSH_ONLY: {

      PSA_push();
      break;
    }
    case ACT_INVALID: {

      PSA_stack_destroy(stack);
      fprintf(stderr, "failed parsing expression (2)\n");
      return ERR_SYNTAX;
    }
    default: {

      fprintf(stderr, "Bad PSA_lookup_table!!!\n");
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