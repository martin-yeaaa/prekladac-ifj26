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
#include <stdbool.h>
#include <stdio.h>
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

static ErrCode PSA_stack_init(PSAStack *stack) {

  // Allocate memory for stack node
  PSAStack new_stack = malloc(sizeof(struct psa_stack_st));
  if (new_stack == NULL) {
    RETURN_ON_ERROR(ERR_INTERNAL, "memory allocation for PSA stack failed");
  }

  // Initialization
  new_stack->previous = NULL;
  new_stack->val = PS_BEGIN;
  new_stack->ast_node = NULL;

  // Pass pointer to variable outside
  *stack = new_stack;
  return SUCCESS;
}

static void PSA_stack_destroy(PSAStack stack) {

  // Cycle through nodes
  while (stack->previous != NULL) {

    PSAStack to_delete = stack;
    // Move stack further
    stack = stack->previous;

    // Delete AST if available
    if (to_delete->ast_node != NULL)
      AST_destroy_tree(to_delete->ast_node);

    free(to_delete);
  }

  // Last node
  if (stack->ast_node != NULL)
    AST_destroy_tree(stack->ast_node);

  free(stack);
}

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

// Enum for deciding which function to use
// Actions represent the < > = symbols inside PSA table
typedef enum psa_actions {
  S,         // ACT_SHIFT, <
  R,         // ACT_REDUCE, >
  P,         // ACT_PUSH_ONLY, =
  I,         // ACT_INVALID, empty slot
  ACT_WRONG, // For detecting my programming errors
} PSAAction;

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

static bool is_terminal(PSAVal value) {
  // Check if value is not one of the only non-terminals
  return (value != PS_EXPR && value != PS_RULE_START);
}

static PSAAction PSA_lookup_table(PSAStack stack, PSAVal input) {

  // We need to compare first terminal from top of stack
  PSAVal term;
  PSAStack search = stack;
  while (is_terminal(search->val) == false) {
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

static ErrCode PSA_push(PSAStack *stack, PSAVal input, ASTNode ast_node) {

  // Very similar to PSA_stack_init hmmmmm
  // Allocate memory for stack node
  PSAStack new_node = malloc(sizeof(struct psa_stack_st));
  if (new_node == NULL) {
    RETURN_ON_ERROR(ERR_INTERNAL, "memory allocation for PSA stack failed");
  }

  // Initialization
  new_node->previous = stack;
  new_node->val = input;
  new_node->ast_node = ast_node;

  // Pass pointer to variable outside
  *stack = new_node;
  return SUCCESS;
}

static ErrCode PSA_shift(PSAStack *stack, PSAVal input, ASTNode ast_node) {

  // Replace top terminal "a" with "a<", and push terminal from input
  // < in my code is PS_RULE_START
  // We need to compare first terminal from top of stack

  PSAStack search = (*stack);
  ErrCode err;

  if (is_terminal(search->val)) {

    // Terminal on top of stack
    PSA_push(stack, PS_RULE_START, NULL); // "<"
    RETURN_ON_ERROR(err, "out of memory");

  } else {
    // Stack has at least 2 nodes in it
    assert(search->previous != NULL);

    // Replace top terminal "a" with "a<"
    // Find top terminal
    while (is_terminal(search->previous->val) == false) {

      search = search->previous;

      // We should find at least PS_BEGIN, if we don't, the stack is invalid
      assert(search->previous != NULL);
    }

    // Create new node for "<"
    PSAStack rule_start_node;
    err = PSA_stack_init(&rule_start_node);
    RETURN_ON_ERROR(err, "out of memory");
    // Change PSA value manually
    rule_start_node->val = PS_RULE_START;

    // Insert it between search and search->previous
    rule_start_node->previous = search->previous;
    search->previous = rule_start_node;
  }
  // Push terminal from input
  err = PSA_push(stack, input, ast_node);
  RETURN_ON_ERROR(err, "out of memory");

  return SUCCESS;
}

static ErrCode PSA_reduce(PSAStack *stack) {

  // PSA reduction rules have at most 3 "values" on the right
  // For example E -> E+E
  PSAStack first_node = NULL;
  PSAStack second_node = NULL;
  PSAStack third_node = NULL;
  ErrCode err = SUCCESS;

  // Helper to go through stack
  PSAStack start_finder = (*stack);

  // "<" cannot be on the top
  if (start_finder->val == PS_RULE_START)
    return ERR_SYNTAX;

  // Fill part of rule
  third_node = start_finder;

  // Move helper
  start_finder = start_finder->previous;

  if (start_finder->val == PS_RULE_START) {
    // We found "<", match rule
    switch (third_node->val) {
    case PS_IDENTIFIER:
    case PS_INT:
    case PS_DOUBLE:
    case PS_STRING: {
      // E -> i
      // E -> VAL_STRING
      // E -> VAL_INT
      // E -> VAL_DOUBLE

      // Get rid of "<" by replacing it with the reduced E
      start_finder->ast_node = third_node->ast_node;
      start_finder->val = PS_EXPR;

      // Destroy the rest
      third_node->previous = NULL;
      third_node->ast_node = NULL;
      PSA_stack_destroy(third_node);

      return SUCCESS;
    }
    default: {
      // No matching rule found
      return ERR_SYNTAX;
    }
    }
  }

  // Fill part of rule
  second_node = start_finder;

  // Move helper again
  start_finder = start_finder->previous;

  if (start_finder->val == PS_RULE_START) {
    // We found "<", match rule
    // This time the rule has 2 "values on the right"

    // E -> +E or E -> -E
    if ((second_node->val == PS_PLUS && third_node->val == PS_EXPR) ||
        (second_node->val == PS_MINUS && third_node->val == PS_EXPR)) {

      // Make tree
      err = AST_node_add_child(second_node->ast_node, third_node->ast_node);
      RETURN_ON_ERROR(err, "out of memory");
    }

    // E -> ()  Unit !!!
    else if (second_node->val == OPEN_BRACKET &&
             third_node->val == CLOSE_BRACKET) {

      // Make Unit literal node!
      ASTNode unit_lit;
      err = AST_init_node(&unit_lit, NODE_LITERAL, DT_UNIT, NULL);
      RETURN_ON_ERROR(err, "failed initializing AST node");

      // Assign to second node
      second_node->ast_node = unit_lit;

    } else {
      // No matching rule found
      return ERR_SYNTAX;
    }

    // Get rid of "<" by replacing it with the reduced E
    start_finder->ast_node = second_node->ast_node;
    start_finder->val = PS_EXPR;

    // Destroy the rest
    third_node->previous = NULL;
    third_node->ast_node = NULL;
    PSA_stack_destroy(third_node);

    second_node->previous = NULL;
    second_node->ast_node = NULL;
    PSA_stack_destroy(second_node);

    return SUCCESS;
  }

  // Fill part of rule
  first_node = start_finder;

  // Move helper again
  start_finder = start_finder->previous;

  if (start_finder->val == PS_RULE_START) {
    // We found "<", match rule
    // This time the rule has 3 "values on the right"

    // E -> E+E
    // E -> E-E
    // E -> E*E
    // E -> E/E
    // E -> (E)

    // E -> E > E
    // E -> E >= E
    // E -> E < E
    // E -> E <= E
    // E -> E == E
    // E -> E != E

    if (first_node->val == PS_OPEN_BRACKET && second_node->val == PS_EXPR &&
        third_node->val == PS_CLOSE_BRACKET) {
      // E -> (E)
      // Do nothing, just pass
    }

    else if (first_node->val == PS_EXPR && third_node->val == PS_EXPR) {
      // E -> E ? E
      switch (second_node->val) {

      case PS_PLUS:
      case PS_MINUS:
      case PS_MULT:
      case PS_DIV: {

        // First add value on left side to keep correct order
        err = AST_node_add_child(second_node->ast_node, first_node->ast_node);
        RETURN_ON_ERROR(err, "out of memory");
        first_node->ast_node = NULL; // Remove useless pointer

        // Then right side
        err = AST_node_add_child(second_node->ast_node, third_node->ast_node);
        RETURN_ON_ERROR(err, "out of memory");
        third_node->ast_node = NULL; // Remove useless pointer
      }

      case PS_LESS:
      case PS_LESS_EQ:
      case PS_GREATER:
      case PS_GREATER_EQ:
      case PS_EQ:
      case PS_NOT_EQ: {

        // AST specification expects expressions with relation operator to be
        // created a little bit weirdly

        // Both children of relation operator must be wrapped in expression node

        // Left side
        ASTNode expr_left;
        err = AST_init_node(&expr_left, NODE_EXPR, DT_UNDEF, NULL);
        RETURN_ON_ERROR(err, "out of memory");
        err = AST_node_add_child(expr_left, first_node->ast_node);
        if (err != SUCCESS) {
          AST_destroy_tree(expr_left);
          RETURN_ON_ERROR(err, "out of memory");
        }
        // Reassign
        first_node->ast_node = expr_left;

        // Right side
        ASTNode expr_right;
        err = AST_init_node(&expr_right, NODE_EXPR, DT_UNDEF, NULL);
        RETURN_ON_ERROR(err, "out of memory");
        err = AST_node_add_child(expr_right, third_node->ast_node);
        if (err != SUCCESS) {
          AST_destroy_tree(expr_right);
          RETURN_ON_ERROR(err, "out of memory");
        }
        // Reassign
        third_node->ast_node = expr_right;

        // Then we can add them as children to relation operator

        // First add value on left side to keep correct order
        err = AST_node_add_child(second_node->ast_node, first_node->ast_node);
        RETURN_ON_ERROR(err, "out of memory");
        first_node->ast_node = NULL; // Remove useless pointer

        // Then right side
        err = AST_node_add_child(second_node->ast_node, third_node->ast_node);
        RETURN_ON_ERROR(err, "out of memory");
        third_node->ast_node = NULL; // Remove useless pointer
      }
      default: {
        // No matching rule found
        return ERR_SYNTAX;
      }
      }

    } else {
      // No matching rule found
      return ERR_SYNTAX;
    }

    // Get rid of "<" by replacing it with the reduced E
    start_finder->ast_node = second_node->ast_node;
    start_finder->val = PS_EXPR;

    // Destroy the rest
    third_node->previous = NULL;
    third_node->ast_node = NULL;
    PSA_stack_destroy(third_node);

    second_node->previous = NULL;
    second_node->ast_node = NULL;
    PSA_stack_destroy(second_node);

    first_node->previous = NULL;
    first_node->ast_node = NULL;
    PSA_stack_destroy(first_node);

    return SUCCESS;
  } else {
    // There is no longer rule
    return ERR_SYNTAX;
  }
}

static ErrCode init_respective_AST_node(ASTNode *ast_node, Token *token) {

  NodeType node_type;
  DataType data_type;

  // Initialize node_type and data_type based on token type
  switch (token->type) {
  case IDENTIFIER: {
    node_type = NODE_IDENTIFIER;
    data_type = DT_UNDEF;
    break;
  }
  case VAL_INT: {
    node_type = NODE_LITERAL;
    data_type = DT_INTEGER;
    break;
  }
  case VAL_DOUBLE: {
    node_type = NODE_LITERAL;
    data_type = DT_DOUBLE;
    break;
  }
  case VAL_STRING: {
    node_type = NODE_LITERAL;
    data_type = DT_STRING;
    break;
  }
  // No Unit token
  case OP_MULT: {
    node_type = NODE_MUL;
    data_type = DT_UNDEF;
    break;
  }
  case OP_DIV: {
    node_type = NODE_DIV;
    data_type = DT_UNDEF;
    break;
  }
  case OP_PLUS: {
    node_type = NODE_ADD;
    data_type = DT_UNDEF;
    break;
  }
  case OP_MINUS: {
    node_type = NODE_SUB;
    data_type = DT_UNDEF;
    break;
  }

  case OP_LESS: {
    node_type = NODE_LT;
    data_type = DT_BOOL;
    break;
  }
  case OP_LESS_EQ: {
    node_type = NODE_LT_EQ;
    data_type = DT_BOOL;
    break;
  }
  case OP_GREATER: {
    node_type = NODE_GT;
    data_type = DT_BOOL;
    break;
  }
  case OP_GREATER_EQ: {
    node_type = NODE_GT_EQ;
    data_type = DT_BOOL;
    break;
  }
  case OP_EQ: {
    node_type = NODE_EQ;
    data_type = DT_BOOL;
    break;
  }
  case OP_NOT_EQ: {
    node_type = NODE_NOT_EQ;
    data_type = DT_BOOL;
    break;
  }
  case OPEN_BRACKET:
  case CLOSE_BRACKET:
  case SEMICOLON:
  case NEW_LINE: {
    // No node for these types
    // Just don't forget to make Unit node later!
    *ast_node = NULL;
    return SUCCESS;
  }
  default: {
    fprintf(stderr,
            "Impossible! I made a mistake! (init_respective_AST_node)\n");
    return ERR_INTERNAL;
  }
  }

  // Initialize the node
  ErrCode err = AST_init_node(ast_node, node_type, data_type, token->value);
  RETURN_ON_ERROR(err, "failed initializing AST node");

  return SUCCESS;
}

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

ErrCode parse_expr(ASTNode parent, Token **cur_token_ptr, bool is_cond) {

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

    // Token is valid for expression, make AST node
    ASTNode ast_node;
    err = init_respective_AST_node(&ast_node, cur_token);
    if (err != SUCCESS) {
      PSA_stack_destroy(stack);
      RETURN_ON_ERROR(err, "inside expression");
    }

    // Figure out what to do
    PSAAction act = PSA_lookup_table(stack, input);
    // Sanity check
    assert(act != ACT_WRONG);

    // Match action
    switch (act) {
    case S: {

      err = PSA_shift(&stack, input, ast_node);
      // Token parsed, get next
      cur_token = get_next_token();
      break;
    }

    case R: {

      err = PSA_reduce(&stack);
      break;
    }

    case P: {

      err = PSA_push(&stack, input, ast_node);
      // Token parsed, get next
      cur_token = get_next_token();
      break;
    }
    case I: {

      PSA_stack_destroy(stack);
      fprintf(stderr, "failed parsing expression, malformed expression\n");
      return ERR_SYNTAX;
    }
    default: {
      // This shouldn't happen
      PSA_stack_destroy(stack);
      fprintf(stderr, "Bad parse_expr!!!\n");
      return ERR_INTERNAL;
    }
    }

    if (err != SUCCESS) {
      PSA_stack_destroy(stack);
      RETURN_ON_ERROR(err, "failed parsing expression");
    }
  } // while parsing not done

  // Exited the loop, which means there is $E on the stack!
  // We will wrap the E expression in NODE_EXPR or NODE_COND,
  // because semantics module requires it

  NodeType w_node_type;
  DataType w_data_type;

  if (is_cond == true) {
    w_node_type = NODE_COND;
    w_data_type = DT_BOOL;
  } else {
    w_node_type = NODE_EXPR;
    w_data_type = DT_UNDEF;
  }

  // Make wrap node
  ASTNode wrap_node;
  err = AST_init_node(&wrap_node, w_node_type, w_data_type, NULL);
  if (err != SUCCESS) {
    PSA_stack_destroy(stack);
    RETURN_ON_ERROR(err, "out of memory, so close :(");
  }

  // Add E as child of wrap node
  err = AST_node_add_child(wrap_node, stack->ast_node);
  if (err != SUCCESS) {
    PSA_stack_destroy(stack);
    AST_destroy_tree(wrap_node);
    RETURN_ON_ERROR(err, "out of memory, so close :((");
  }
  stack->ast_node = NULL; // Remove pointer

  // Add wrap node as child of arg "parent"!
  err = AST_node_add_child(parent, wrap_node);
  if (err != SUCCESS) {
    PSA_stack_destroy(stack);
    AST_destroy_tree(wrap_node);
    RETURN_ON_ERROR(err, "out of memory, so close :(((");
  }

  // Cleanup
  PSA_stack_destroy(stack);

  // Update foreign cur_token before returning
  *cur_token_ptr = cur_token;

  return SUCCESS;
}
