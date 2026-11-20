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
ErrCode check_other_semantics(ASTNode root);

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
int find_func_calls(ASTNode func_body, BSTNode *call_list);

/**
 * @brief Fills the symbol table with function symbols.
 * @param root Root of AST. Must be valid.
 * @param st Pointer to FRESHLY INITIALIZED symbol tables structure.
 * @return `SUCCESS` if definitions are successful, otherwise one of the
 * following error codes:
 * `ERR_UNDEF_SYM` if @main function is missing,
 * `ERR_REDEFINITION` if a function is redefined,
 * `ERR_FUN_ARGS` if @main function has invalid parameters,
 * `ERR_INTERNAL` if memory allocation fails.
 */
ErrCode define_func_symbols(ASTNode root, SymbTables *st);

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
ErrCode define_variable_symbol(ASTNode idtf, SymbType sb_type, SymbTables *st);

#endif
