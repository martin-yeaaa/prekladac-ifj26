#include "codegen.h"
#include <stdio.h>

// function to calculate math and push values to stack
void generate_expression(ASTNode *node) {
  if (node == NULL) {
    return;
  }

  // first go to left and right nodes
  generate_expression(node->left);
  generate_expression(node->right);

  // check what type of node we have
  if (node->type == NODE_INT) {
    printf("PUSHS int@%d\n", node->int_val);
  } else if (node->type == NODE_DOUBLE) {
    printf("PUSHS float@%a\n", node->double_val);
  } else if (node->type == NODE_ADD) {
    printf("ADDS\n");
  } else if (node->type == NODE_SUB) {
    printf("SUBS\n");
  } else if (node->type == NODE_MUL) {
    printf("MULS\n");
  } else if (node->type == NODE_DIV) {
    printf("IDIVS\n");
  }
}

// main function that starts generating code
void generate_code(ASTNode *root) {
  // mandatory header at the start
  printf(".IFJcode26\n");

  // helper variable for results
  printf("DEFVAR GF@temp_result\n");

  if (root == NULL) {
    return;
  }

  // if root is just a print command for now
  if (root->type == NODE_WRITE) {
    generate_expression(root->left);
    printf("POPS GF@temp_result\n");
    printf("WRITE GF@temp_result\n");
  }
}