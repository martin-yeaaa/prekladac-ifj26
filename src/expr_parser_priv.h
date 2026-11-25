// Project: prekladac-ifj26
// File: expr_parser_priv.h
// Author: Martin Vaško (xvaskom00)

// Header file for declarations of internal functions only visible to
// expr_parser.c

#ifndef EXPR_PARSER_PRIV_H
#define EXPR_PARSER_PRIV_H

#include "ast_module.h"
#include "lexer.h"
#include <stdbool.h>

// Possible values of PSA nodes
// Most of them represent an equivalent TokenType, but not all
typedef enum psa_values {

  PS_BEGIN, // In table: $ (on stack)
  PS_END,   // In table: $ (in input)

  // The only non-terminals
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
  ASTNode ast_node;
  PSAVal val;

} *PSAStack;

// Enum for deciding which function to use
// Actions represent the < > = symbols inside PSA table
typedef enum psa_actions {
  S, // ACT_SHIFT, <
  R, // ACT_REDUCE, >
  P, // ACT_PUSH_ONLY, =
  I, // ACT_INVALID, empty slot
} PSAAction;

/**
 * @brief Initializes stack for precedence syntax analysis,
 * and inserts value `PS_BEGIN`.
 * @param stack Pointer to declared uninitialized `PSAStack` variable.
 * @return `SUCCESS` if it succeeds, otherwise `ERR_INTERNAL`.
 */
static ErrCode PSA_stack_init(PSAStack *stack);

/**
 * @brief Frees stack from memory. It will become unusable.
 * @param stack Stack to free.
 */
static void PSA_stack_destroy(PSAStack stack);

/**
 * @brief Tries to convert value of type `TokenType` to `PSAVal`.
 * @param token_type Value of type `TokenType`.
 * @return Equivalent `PSAVal`, or `PS_UNKNOWN`.
 */
static PSAVal tokentype_to_psaval(TokenType token_type);

/**
 * @param value Value to check.
 * @return `true` if it is terminal, otherwise `false`.
 */
static bool is_terminal(PSAVal value);

/**
 * @brief Gets the number of row/column of PSA table based on `PSAVal`.
 * @param val Value whose index we are looking for.
 * @return Index `0` to `6` if found, `-1` otherwise.
 */
static int get_PSA_table_idx(PSAVal val);

/**
 * @brief Gets the appropriate action from PSA table.
 * @param stack Stack currently used in PSA.
 * @param input `PSAVal` of current input.
 * @return Appropriate `PSAAction` value.
 */
static PSAAction PSA_lookup_table(PSAStack stack, PSAVal input);

/**
 * @brief Does the standard PSA push "=" action.
 * @param stack Stack currently used in PSA.
 * @param input `PSAVal` of current input.
 * @param ast_node Initialized AST node, or NULL.
 * @return `SUCCESS` on success, otherwise `ERR_INTERNAL`
 */
static ErrCode PSA_push(PSAStack *stack, PSAVal input, ASTNode ast_node);

/**
 * @brief Does the standard PSA shift "<" action.
 * @param stack Stack currently used in PSA.
 * @param input `PSAVal` of current input.
 * @param ast_node Initialized AST node, or NULL.
 * @return `SUCCESS` on success, otherwise `ERR_INTERNAL`
 */
static ErrCode PSA_shift(PSAStack *stack, PSAVal input, ASTNode ast_node);
/**
 * @brief Does the standard PSA reduce ">" action.
 * @param stack Stack currently used in PSA.
 * @return `SUCCESS` on success, otherwise `ERR_SYNTAX` or `ERR_INTERNAL`
 */
static ErrCode PSA_reduce(PSAStack *stack);

/**
 * @brief Checks if syntactic analysis is finished.
 * @param stack Initialized `PSAStack` variable.
 * @return `true` if SA is finished, otherwise `false`.
 */
static bool PSA_finished(PSAStack stack);

/**
 * @brief Initialized AST node based on token type.
 * @param ast_node Pointer to declared uninitialized node.
 * @param token Pointer to token.
 * @return `SUCCESS` if initializing succeeds, or `ERR_INTERNAL`.
 */
static ErrCode init_respective_AST_node(ASTNode *ast_node, Token *token);

#endif
