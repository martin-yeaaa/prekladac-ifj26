

// TODO MAYBE USE IF LONG ENOUGH

#ifndef EXPR_PARSER_PRIV_H
#define EXPR_PARSER_PRIV_H

#include "ast_module.h"
#include "lexer.h"

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

/**
 * @brief Initializes stack for precedence syntax analysis,
 * and inserts value `PS_BEGIN`.
 * @param stack Pointer to declared uninitialized `PSAStack` variable.
 * @return `SUCCESS` if it succeeds, otherwise `ERR_INTERNAL`.
 */
static ErrCode PSA_stack_init(PSAStack *stack);

/**
 * @brief Tries to convert value of type `TokenType` to `PSAVal`.
 * @param token_type Value of type `TokenType`.
 * @return Equivalent `PSAVal`, or `PS_UNKNOWN`.
 */
static PSAVal tokentype_to_psaval(TokenType token_type);

/**
 * @brief Gets the number of row/column of PSA table based on `PSAVal`.
 * @param val Value whose index we are looking for.
 * @return Index `0` to `6` if found, `-1` otherwise.
 */
static int get_PSA_table_idx(PSAVal val);

/**
 * @brief Checks if syntactic analysis is finished.
 * @param stack Initialized `PSAStack` variable.
 * @return `true` if SA is finished, otherwise `false`.
 */
static bool PSA_finished(PSAStack stack);

#endif
