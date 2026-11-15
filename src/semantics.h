// Project: prekladac-ifj26
// File: ast.h
// Authors:
// - Martin Vaško (vaskom00)
// - Lukáš Dvořák (xdvoral00)

// Header file for abstract syntax tree = AST and semantic analysis

#ifndef SEMANTICS_H
#define SEMANTICS_H

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

  NODE_IF,
  NODE_WHILE,

  NODE_WRITE, // TODO WHAT IS THIS

  NODE_FUNC_CALL, // call a function
  NODE_RETURN,    // return from function

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
  int child_node_count;
  struct ast_node_struct **child_node_arr;

} *ASTNode;

/**
 * @brief Creates a new node for AST.
 * @param node Pointer to declared uninitialized node.
 * @param node_type What this node represents.
 * @param data_type Type of data this node holds.
 * @param node_data Value of literal, or identifier string.
 * This is ignored when it's not needed.
 * @return `0` on success, `99` on memory allocation failure,
 * `6` if conversion fails while assigning data.
 */
int AST_init_node(ASTNode *node, NodeType node_type, DataType data_type,
                  char *node_data);

/**
 * @brief Creates a tree from a parent node and children nodes.
 * @param parent AST node that should be the parent, not `NULL`.
 * @param child_count Amount of children nodes, must be at least 1.
 * @param child_arr Array of children nodes, "correctly sorted", not `NULL`.
 * @return `0` on success, `99` on memory allocation failure.
 */
int AST_make_tree(ASTNode parent, int child_count, ASTNode *child_arr);

/**
 * @brief Fills the symbol table, does all type checking.
 * @param root Root of AST.
 * @param st Pointer to declared uninitialized symbol tables structure.
 * @return Error code. TODO
 */
int semantic_analysis(ASTNode root, SymbTables *st);

#endif
