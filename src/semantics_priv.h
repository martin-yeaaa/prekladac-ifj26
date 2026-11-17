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
 * @brief Goes through children of `node`, looks for node with `searched_type`.
 * @param node Node whose children will be checked. Not `NULL`.
 * @param searched_type Value of `NodeType` which is searched,
 * @return Node with type if found, otherwise `NULL`.
 */
static ASTNode get_node_of_type(ASTNode node, NodeType searched_type);

/**
 * @brief Goes through tree and looks for invalid return statements.
 * @param root Root of AST. Must be valid.
 * @return `SUCCESS` if everything is valid,
 * `ERR_OTHER_SEMANTIC` if an invalid return statement is found.
 */
ErrCode check_return_stmts(ASTNode root);

/**
 * @brief Recursively searches block node for return statements.
 * @param block Node of type `NODE_BLOCK`.
 * @return `true` if return is found, otherwise `false`.
 */
static bool contains_return(ASTNode block);

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

#endif
