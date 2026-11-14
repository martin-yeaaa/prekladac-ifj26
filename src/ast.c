// Project: prekladac-ifj26
// File: ast.c
// Author: Martin Vaško (vaskom00)

// Implementation of abstract syntax tree = AST

#include "ast.h"
#include "symtable.h"

ASTNode AST_create_node(NodeType node_type, DataType data_type,
                        NodeData node_data) {

  ASTNode new_node = malloc(sizeof(struct ast_node_struct));
  if (new_node == NULL)
    return NULL;

  return new_node;
}

ASTNode AST_create_tree(ASTNode parent, int child_count, ASTNode *child_arr);