// Project: prekladac-ifj26
// File: semantics_priv.h
// Author: Martin Vaško (vaskom00)

#ifndef SEMANTICS_PRIV_H
#define SEMANTICS_PRIV_H

#include "error_codes.h"
#include "semantics.h"
#include <stdbool.h>

// Declarations of internal functions

/**
 * @brief Converts string to 32 bit int and sets it as node data.
 * @param node Node into which data will be inserted. Not `NULL`.
 * @param data_string Data to be converted and saved. Not `NULL`.
 * @return `SUCCESS` on success,
 * `ERR_INCOMPATIBLE_TYPE` if conversion fails.
 */
static ErrCode set_integer_node_data(ASTNode node, char *data_string);

/**
 * @brief Converts string to 64 bit float and sets it as node data.
 * @param node Node into which data will be inserted. Not `NULL`.
 * @param data_string Data to be converted and saved. Not `NULL`.
 * @return `SUCCESS` on success,
 * `ERR_INCOMPATIBLE_TYPE` if conversion fails.
 */
static ErrCode set_double_node_data(ASTNode node, char *data_string);

/**
 * @brief Copies string into node data.
 * @param node Node into which data will be inserted. Not `NULL`.
 * @param data_string Data to be copied. Not `NULL`.
 * @return `SUCCESS` on success,
 * `ERR_INTERNAL` on memory allocation failure,
 */
static ErrCode set_string_node_data(ASTNode node, char *data_string);

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

static ErrCode infer_or_check_types(ASTNode lhs, ASTNode rhs, ASTNode parent);

static ErrCode convert_int_to_double(ASTNode child, ASTNode parent);

static ErrCode resolve_any_func_body(ASTNode func_body, SymbTables *st);
static ErrCode resolve_any_stmt(ASTNode stmt, SymbTables *st);
static ErrCode resolve_any_rhs(ASTNode rhs_node, SymbTables *st);
static ErrCode resolve_any_subexpr(ASTNode subexpr, SymbTables *st);

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
static ErrCode resolve_func_def(ASTNode func_def, SymbTables *st);
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

// Declarations to sort out

#endif
