#ifndef AST_H
#define AST_H

#include "symtable.h"
#include <stdbool.h>
#include <stddef.h>

// Basic node types for AST
typedef enum {

  // Probably wrong
  NODE_INT,
  NODE_DOUBLE,
  NODE_STRING,

  NODE_IDENTIFIER,
  NODE_CONST,

  NODE_ADD,
  NODE_SUB,
  NODE_MUL,
  NODE_DIV,
  NODE_EQ, // equals (==)
  NODE_LT, // less than (<)
  NODE_GT, // greater than (>)
  NODE_WRITE,
  NODE_VAR_DECL, // variable declaration
  NODE_ASSIGN,   // variable assignment (=)
  NODE_VAR,      // variable usage
  NODE_IF,
  NODE_WHILE,

  NODE_BOOL, // true or false value
  NODE_AND,  // logical and (&&)
  NODE_OR,   // logical or (||)
  NODE_NOT,  // logical not (!)
  NODE_LEQ,  // less or equal (<=)
  NODE_GEQ,  // greater or equal (>=)
  NODE_NEQ,  // not equal (!=)

  NODE_FUNC_DEF,  // create a new function (def)
  NODE_FUNC_CALL, // call a function
  NODE_RETURN,    // return from function

  NODE_PROGRAM,     // main root of the whole code
  NODE_LIST_CREATE, // make new list (e.g. List(1, 2))
  NODE_LIST_METHOD, // list.head, list.tail, etc.
  NODE_LIST_CONCAT  // ++ for joining lists together
} NodeType;

// Union for AST node data
union Data {
  int int_val;
  double double_val;
  char *string_val;
  bool bool_val;
};

// Structure for one AST node
typedef struct ast_node_struct {

  NodeType type;

  Data node_val;

  Symbol *symbol_ptr;

  struct ast_node_struct **son_array;

  /*
  struct ASTNode *left;
  struct ASTNode *right;

  struct ASTNode *if_body;   // commands inside IF or WHILE
  struct ASTNode *else_body; // commands inside ELSE branch

  struct ASTNode *next; // next statement in sequence

  bool bool_val;          // value for NODE_BOOL
  struct ASTNode *args;   // arguments when calling a function
  struct ASTNode *params; // parameters when creating a function
  */
} *ASTNode;

// function to create a node so we don't have to write malloc everywhere
ASTNode create_node(NodeType type);

//  ASTNode *cislo1 = create_node(NODE_INT);
//  cislo1->int_val = 5;

//  ASTNode *cislo2 = create_node(NODE_INT);
//  cislo2->int_val = 10;

//  ASTNode *plus = create_node(NODE_ADD);
//  plus->left = cislo1;
//  plus->right = cislo2;

#endif