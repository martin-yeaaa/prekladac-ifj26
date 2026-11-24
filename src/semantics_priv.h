// Project: prekladac-ifj26
// File: semantics_priv.h
// Author: Martin Vaško (vaskom00)

// Declarations of internal functions

#ifndef SEMANTICS_PRIV_H
#define SEMANTICS_PRIV_H

#include "ast_module.h"
#include "error_codes.h"
#include "semantics.h"
#include <stdbool.h>

/**
 * @brief Goes through tree and looks for invalid return statements or
 * recursion.
 * @param root Root of AST. Must be valid.
 * @return `SUCCESS` if everything is valid,
 * `ERR_OTHER_SEMANTIC` if an invalid return statement or recursion is found.
 */
static ErrCode check_other_semantics(ASTNode root);

/**
 * @brief Recursively searches block node for return statements.
 * @param block Node of type `NODE_BLOCK`.
 * @return `true` if return is found, otherwise `false`.
 */
static bool contains_return(ASTNode block);

/**
 * @brief Detects direct or indirect recursion.
 * @param func_body Body node of checked function
 * @param func_name Name of checked function.
 * @param root Root of whole AST so it can check indirect recursion.
 * @return `SUCCESS` if NO recursion is found,
 * `ERR_OTHER_SEMANTIC` if recursion IS found,
 * `ERR_INTERNAL` if memory allocation fails.
 */
static ErrCode check_recursion(ASTNode func_body, char *func_name,
                               ASTNode root);
/**
 * @brief Recursively searches for function calls, and if found, adds
 * identifiers of called functions to call_list
 * @param func_body
 * @param call_list
 * @return `-1` if an error occurs, or amount of found function calls if
 * everything goes right.
 */
static int find_func_calls(ASTNode func_body, BSTNode *call_list);

/**
 * @brief Fills the symbol table with function symbols, also saves func_def
 * nodes into global func_def_list.
 * @param root Root of AST. Must be valid.
 * @param st Pointer to FRESHLY INITIALIZED symbol tables structure.
 * @return `SUCCESS` if definitions are successful, otherwise one of the
 * following error codes:
 * `ERR_UNDEF_SYM` if @main function is missing,
 * `ERR_REDEFINITION` if a function is redefined,
 * `ERR_FUN_ARGS` if @main function has invalid parameters,
 * `ERR_INTERNAL` if memory allocation fails.
 */
static ErrCode define_func_symbols(ASTNode root, SymbTables *st);

/**
 * @brief Checks if data types are compatible, or if right side has undefined
 * data type, it inherits the data type from the left side. Can do implicit
 * conversion of integer to double.
 * @param lhs Identifier type node on the left side.
 * @param rhs Expression or function call node on the right side.
 * @param parent Parent node of both sides.
 * @return `SUCCESS` if types are compatible, error code otherwise.
 */
static ErrCode infer_or_check_types(ASTNode lhs, ASTNode rhs, ASTNode parent);

/**
 * @brief Does conversion of integer data type to double by inserting a special
 * kind of type between parent and child node.
 * @param child Node with integer data type to convert.
 * @param parent Parent node of `child`.
 * @return `SUCCESS` if conversion succeeds, error code otherwise.
 */
static ErrCode convert_int_to_double(ASTNode child, ASTNode parent);

/**
 * @brief Fills the symbol table with local symbols of variables and function
 * parameters, resolves types, checks for semantic errors.
 * @param root Root of AST. Must be valid.
 * @param st Pointer to symbol tables structure filled with function symbols.
 * @return `SUCCESS` if definitions are successful, otherwise one of the
 * following error codes:
 * `ERR_UNDEF_SYM` if a symbol is undefined,
 * `ERR_REDEFINITION` if a symbol is redefined,
 * `ERR_FUN_ARGS` if func call has wrong arguments,
 * `ERR_INCOMPATIBLE_TYPE` on type inncompatibility.
 * `ERR_INVALID_ASSIGN` on invalid assingment,
 * `ERR_INTERNAL` if memory allocation fails.
 */
static ErrCode resolve_AST(ASTNode root, SymbTables *st);

// resolve_any_x functions choose the correct resolve_x function depending on
// node type.
static ErrCode resolve_any_func_body(ASTNode func_body, SymbTables *st);
static ErrCode resolve_any_stmt(ASTNode stmt, SymbTables *st);
static ErrCode resolve_any_rhs(ASTNode rhs_node, SymbTables *st);
static ErrCode resolve_any_subexpr(ASTNode subexpr, SymbTables *st);

// resolve_x functions do the same as resolve_AST, just personalized based on
// node type.
static ErrCode resolve_func_def(ASTNode func_def, SymbTables *st);
// `check_end_if` - boolean, set to `true` if the last statement should be
// checked if it's if statement and do the appropriate checks.
static ErrCode resolve_block(ASTNode block, SymbTables *st, bool check_end_if);
static ErrCode resolve_func_call(ASTNode func_call, SymbTables *st);
static ErrCode resolve_arg(ASTNode arg, SymbTables *st);
static ErrCode resolve_expression(ASTNode expr, SymbTables *st);
static ErrCode resolve_var_def(ASTNode var_def, SymbTables *st);
static ErrCode resolve_val_def(ASTNode val_def, SymbTables *st);
static ErrCode resolve_assignment(ASTNode assign, SymbTables *st);
static ErrCode resolve_binary_op(ASTNode bin_op, SymbTables *st);
static ErrCode resolve_unary_op(ASTNode un_op, SymbTables *st);
static ErrCode resolve_rhs_var_id(ASTNode idtf, SymbTables *st);
static ErrCode resolve_condition(ASTNode cond, SymbTables *st);
// `is_func_result` - boolean, set to `true` if the if statement is a result of
// function and the appropriate checks should be done.
static ErrCode resolve_if(ASTNode if_stmt, SymbTables *st, bool is_func_result);
static ErrCode resolve_while(ASTNode while_stmt, SymbTables *st);
static ErrCode resolve_return(ASTNode return_stmt, SymbTables *st);

/**
 * @brief Checks if block is guaranteed to execute a return statement at some
 * point. Also recursively checks blocks inside if statements. Doesn't consider
 * blocks inside while loops.
 * @param block Node of type `NODE_BLOCK`
 * @return `true` if block is guaranteed hit return, `false` if it's not trivial
 * to tell whether a return will be executed or not.
 */
static bool block_ends_with_return(ASTNode block);

/**
 * @brief Properly inserts variable symbol into symbol table.
 * @param block ASTNode of type NODE_IDENTIFIER. Must be valid.
 * @param sb_type Type of symbol (SB_VAL or SB_VAR)
 * @param st Pointer to symbol tables structure. Expects to be in correct scope!
 * @return `SUCCESS` if definitions are successful, otherwise one of the
 * following error codes:
 * `ERR_REDEFINITION` if a symbol is redefined,
 * `ERR_INTERNAL` if memory allocation fails.
 */
static ErrCode define_variable_symbol(ASTNode idtf, SymbType sb_type,
                                      SymbTables *st);

#endif
