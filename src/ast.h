// Project: prekladac-ifj26
// File: ast.h
// Authors:
// - Martin Vaško (vaskom00)
// - Lukáš Dvořák (xdvoral00)

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
  NODE_ASSIGN,

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

  NODE_WRITE,

  NODE_FUNC_CALL, // call a function
  NODE_RETURN,    // return from function

  /* TODO delete if BOOLTHEN unsupported
  NODE_BOOL, // true or false value
  NODE_AND,  // logical and (&&)
  NODE_OR,   // logical or (||)
  NODE_NOT,  // logical not (!)
*/

  /* TODO delete if LISTS unsupported
  NODE_LIST_CREATE, // make new list (e.g. List(1, 2))
  NODE_LIST_METHOD, // list.head, list.tail, etc.
  NODE_LIST_CONCAT,  // ++ for joining lists together
  */

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

  // This can be a value of a constant, or idnetifier name, or other maybe TODO
  NodeData node_val;

  // If node has identifier, this points to matching symbol in symtable
  Symbol *symbol_ptr;

  // Son nodes, there can be many
  int son_node_count;
  struct ast_node_struct **son_node_arr;

} *ASTNode;

/**
 * bruh
 */
ASTNode create_node(NodeType type);

#endif
