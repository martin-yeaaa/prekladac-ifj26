// Project: prekladac-ifj26
// File: semantics.h
// Authors:
// - Martin Vaško (vaskom00) - everything except initial file structure
// - Lukáš Dvořák (xdvoral00) - initial file structure

// Header file for abstract syntax tree = AST and semantic analysis

#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "error_codes.h"
#include "symtable.h"
#include <stdbool.h>
#include <stdint.h>
// #include <stddef.h> TODO delete if useless

// Node types for AST nodes
typedef enum {

  // Root node of the AST
  NODE_PROGRAM,

  // Node types for literals
  NODE_LITERAL,

  NODE_IDENTIFIER,
  NODE_ASSIGNMENT,

  // Operators
  // Unary operators
  NODE_UN_PLUS,
  NODE_UN_MINUS,

  // Binary operators
  NODE_ADD,
  NODE_SUB,
  NODE_MUL,
  NODE_DIV,
  // Binary relation operators
  NODE_EQ,     // equals (==)
  NODE_NOT_EQ, // not equal (!=)
  NODE_LT,     // less than (<)
  NODE_LT_EQ,  // less or equal (<=)
  NODE_GT,     // greater than (>)
  NODE_GT_EQ,  // greater or equal (<=)

  // Different kinds of definitions
  NODE_VAR_DEF,
  NODE_VAL_DEF,
  NODE_FUNC_DEF,
  NODE_MAIN_FUNC_DEF,
  NODE_PARAM_LIST,

  NODE_EXPR,

  NODE_FUNC_CALL,
  NODE_ARG_LIST,
  NODE_RETURN, // return from function

  // Indented block types
  NODE_BLOCK,
  NODE_THEN_BLOCK, // if statement - block after then
  NODE_ELSE_BLOCK, // if statement - block after else

  NODE_IF,
  NODE_WHILE,
  NODE_COND, // Condition

} NodeType;

// Union for AST node data
typedef union node_data_union {
  int32_t int_val;
  double double_val;
  char *string_val;
  // Bool must exist internally even without BOOLTHEN
  bool bool_val;
} NodeData;

// Structure for one AST node
typedef struct ast_node_struct {

  // Type matching syntax rule i guess TODO
  NodeType node_type;

  // If node contains:
  // - a literal, it will have data type of literal
  // - a symbol, it will have data type that will be inserted into symbol table
  // - an expression, it will have data type of result
  DataType data_type;

  // If node contains:
  // - a literal, it will hold value of literal
  // - a symbol, it will hold identifier of the symbol
  NodeData data;

  // If node type is a , this points to matching symbol in symtable
  Symbol *symbol_ptr;

  // Children nodes, there can be many
  int child_count;
  int child_limit; // Info for realloc
  struct ast_node_struct **child_arr;

} *ASTNode;

/**
 * @brief Creates a new node for AST.
 * @param node Pointer to declared uninitialized node.
 * @param node_type What this node represents.
 * @param data_type Type of data this node holds.
 * @param node_data Value of literal, or identifier string.
 * This is ignored when it's not needed.
 * @return `SUCCESS` on success,
 * `ERR_INTERNAL` on memory allocation failure,
 * `ERR_INCOMPATIBLE_TYPE` if conversion fails while assigning data.
 */
ErrCode AST_init_node(ASTNode *node, NodeType node_type, DataType data_type,
                      char *node_data);

/**
 * @brief Creates a tree from a parent node and children nodes.
 * @param parent AST node that will be the parent, not `NULL`.
 * @param child AST node that will be the child, not `NULL`.
 * @return `SUCCESS` on success,
 * `ERR_INTERNAL` on memory allocation failure.
 */
ErrCode AST_node_add_child(ASTNode parent, ASTNode child);

/**
 * @brief Fills the symbol table, does all type checking.
 * @param root Root of AST.
 * @param st Pointer to declared UNINITIALIZED symbol tables structure.
 * @return 'SUCCESS' if analysis is successful, otherwise one of the following
 * error codes: `ERR_UNDEF_SYM`, `ERR_REDEFINITION`, `ERR_FUN_ARGS`,
 * `ERR_INCOMPATIBLE_TYPE`, `ERR_INVALID_ASSIGN`, `ERR_OTHER_SEMANTIC`,
 * `ERR_INTERNAL`
 */
ErrCode semantic_analysis(ASTNode root, SymbTables *st);

#endif
