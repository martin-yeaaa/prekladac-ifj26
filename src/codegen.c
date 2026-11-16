// Project: prekladac-ifj26
// File: codegen.c
// Author: Lukáš Dvořák (xdvoral00)

#include "codegen.h"
#include <stdio.h>
#include <string.h> // needed for strcmp

// declaration for functions
void generate_expression(ASTNode node);

// builtin functions
void generate_builtin(ASTNode node) {
  // null check
  if (node == NULL || node->child_count < 2) {
    return;
  }

  // child 0 is identifier, child 1 is arg_list
  char *func_name = node->child_arr[0]->data.string_val;
  ASTNode args = node->child_arr[1];

  if (func_name == NULL || args == NULL) {
    return;
  }

  // print
  if (strcmp(func_name, "print") == 0) {
    // loop for multiple print args
    for (int i = 0; i < args->child_count; i++) {
      generate_expression(args->child_arr[i]);
      printf("POPS GF@temp_result\n");
      printf("WRITE GF@temp_result\n");
    }
    // push nil for return value
    printf("PUSHS nil@nil\n");
  }
  // readint
  else if (strcmp(func_name, "readInt") == 0) {
    printf("READ GF@temp_result int\n");
    printf("PUSHS GF@temp_result\n");
  }
  // readdouble
  else if (strcmp(func_name, "readDouble") == 0) {
    printf("READ GF@temp_result float\n");
    printf("PUSHS GF@temp_result\n");
  }
  // readline
  else if (strcmp(func_name, "readLine") == 0) {
    printf("READ GF@temp_result string\n");
    printf("PUSHS GF@temp_result\n");
  }
  // string length
  else if (strcmp(func_name, "strLength") == 0) {
    if (args->child_count > 0) {
      generate_expression(args->child_arr[0]);
      printf("POPS GF@temp_result\n");
      printf("STRLEN GF@temp_result GF@temp_result\n");
      printf("PUSHS GF@temp_result\n");
    }
  }
  // toint
  else if (strcmp(func_name, "toInt") == 0) {
    if (args->child_count > 0) {
      generate_expression(args->child_arr[0]);
      printf("POPS GF@temp_result\n");
      printf("FLOAT2INT GF@temp_result GF@temp_result\n");
      printf("PUSHS GF@temp_result\n");
    }
  }
  // chr
  else if (strcmp(func_name, "chr") == 0) {
    if (args->child_count > 0) {
      generate_expression(args->child_arr[0]);
      printf("POPS GF@temp_result\n");
      printf("INT2CHAR GF@temp_result GF@temp_result\n");
      printf("PUSHS GF@temp_result\n");
    }
  }
}

// function to calculate math and push values to stack
void generate_expression(ASTNode node) {
  if (node == NULL) {
    return;
  }

  // function call check first
  // prevention for double arguments
  if (node->node_type == NODE_FUNC_CALL) {
    if (node->child_count > 0 && node->child_arr[0]->symbol_ptr != NULL) {
      if (node->child_arr[0]->symbol_ptr->symb_type == SB_BUILTIN) {
        generate_builtin(node);
      }
    }
    // stop here
    return;
  }

  // first go to left (index 0) and right (index 1) nodes if they exist
  if (node->child_count == 2) {
    generate_expression(node->child_arr[0]);
    generate_expression(node->child_arr[1]);
  }
  // unary operation
  else if (node->child_count == 1) {
    generate_expression(node->child_arr[0]);
  }

  // check what type of node we have
  if (node->node_type == NODE_LITERAL) {
    if (node->data_type == DT_INTEGER) {
      printf("PUSHS int@%d\n", node->data.int_val);
    } else if (node->data_type == DT_DOUBLE) {
      printf("PUSHS float@%a\n", node->data.double_val);
    }
  } else if (node->node_type == NODE_IDENTIFIER) {
    // id from symtable
    if (node->symbol_ptr != NULL) {
      printf("PUSHS LF@var_%d\n", node->symbol_ptr->id);
    }
  } else if (node->node_type == NODE_ADD) {
    printf("ADDS\n");
  } else if (node->node_type == NODE_SUB) {
    printf("SUBS\n");
  } else if (node->node_type == NODE_MUL) {
    printf("MULS\n");
  } else if (node->node_type == NODE_DIV) {
    printf("IDIVS\n");
  } else if (node->node_type == NODE_UN_MINUS) {
    // multiply by minus one trick
    printf("PUSHS int@-1\n");
    printf("MULS\n");
  }
}

// function to process statements inside a block
void generate_statement(ASTNode node) {
  if (node == NULL) {
    return;
  }

  if (node->node_type == NODE_FUNC_CALL) {
    // standalone function call
    generate_expression(node);
    // throw away unused result
    printf("POPS GF@temp_result\n");
  } else if (node->node_type == NODE_VAR_DEF ||
             node->node_type == NODE_VAL_DEF) {
    // new variable (child 0 is name, child 1 is value)
    if (node->child_count == 2 && node->child_arr[0]->symbol_ptr != NULL) {
      printf("DEFVAR LF@var_%d\n", node->child_arr[0]->symbol_ptr->id);
      // calc and save value
      generate_expression(node->child_arr[1]);
      printf("POPS LF@var_%d\n", node->child_arr[0]->symbol_ptr->id);
    }
  } else if (node->node_type == NODE_ASSIGNMENT) {
    // variable assignment (child 0 is name, child 1 is value)
    if (node->child_count == 2 && node->child_arr[0]->symbol_ptr != NULL) {
      generate_expression(node->child_arr[1]);
      printf("POPS LF@var_%d\n", node->child_arr[0]->symbol_ptr->id);
    }
  } else {
    // loop through block
    for (int i = 0; i < node->child_count; i++) {
      generate_statement(node->child_arr[i]);
    }
  }
}

// main function that starts generating code
void generate_code(ASTNode root) {
  // mandatory header at the start
  printf(".IFJcode26\n");

  // helper variable for results
  printf("DEFVAR GF@temp_result\n");

  // temp frame for tests
  printf("CREATEFRAME\n");
  printf("PUSHFRAME\n");

  if (root != NULL) {
    generate_statement(root);
  }
}