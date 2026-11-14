// Project: prekladac-ifj26
// File: ast.h
// Authors:
// - Martin Vaško (vaskom00)
// - Lukáš Dvořák (xdvoral00)

// Header file for abstract syntax tree = AST

#ifndef AST_H
#define AST_H

#include "symtable.h"
#include <stdbool.h>
#include <stddef.h>

// Node types for AST nodes
typedef enum {

  // Root node of the AST
  NODE_PROGRAM,

  // Node types for constants
  NODE_INT,
  NODE_DOUBLE,
  NODE_STRING,
  NODE_UNIT,

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
union NodeData {
  int int_val;
  double double_val;
  char *string_val;
  // Bool must exist internally even without BOOLTHEN
  bool bool_val;
};

// Structure for one AST node
typedef struct ast_node_struct {

  // Type matching syntax rule i guess TODO
  NodeType type;

  // This can be a value of a constant, or identifier name, or other maybe TODO
  NodeData node_val;

  // If node has identifier, this points to matching symbol in symtable
  Symbol *symbol_ptr;

  // Children nodes, there can be many
  int child_node_count;
  struct ast_node_struct **child_node_arr;

} *ASTNode;

/**
 * Creates a new node for AST.
 * @param node_type Type of action or "thing" this node should hold.
 * @param data_type Type of data this node holds.
 * @param node_data Value of literal, or identifier string.
 * @return New node, or `NULL` if creation fails.
 */
ASTNode AST_create_node(NodeType node_type, DataType data_type,
                        NodeData node_data);

/**
 * Creates a tree from a parent node and children nodes.
 * @param parent AST node that should be the parent.
 * @param child_count Amount of children nodes.
 * @param child_arr Array of children nodes.
 * @return Parent node, or `NULL` if creation fails.
 */
ASTNode AST_create_tree(ASTNode parent, int child_count, ASTNode *child_arr);

#endif
