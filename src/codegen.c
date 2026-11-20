// Project: prekladac-ifj26
// File: codegen.c
// Author: Lukáš Dvořák (xdvoral00)

// how this file works (in order of calls):
// 1. generate_code() - prints header, temp vars and starts going thru the AST
// 2. generate_statement() - runs commands (var/val, =, if, while, blocks)
// 3. generate_expression() - computes math, strings and comparisons on stack
// 4. generate_builtin() - prints code for builtin functions directly (print,
// read...)
// 5. print_escaped_string() - helper for formatting strings to \XYZ ascii codes

#include "codegen.h"
#include <stdio.h>
#include <string.h> // needed for strcmp when checking builtin function names

// GLOBAL VARIABLES AND DECLARATIONS
// ---------------------------------

// counter for unik labels so if and while jumps dont get the same name
// if two ifs had the same label name, the ifjcode26 interpreter would crash
int label_id = 0;

// forward declaration because generate_builtin and generate_expression call
// each other (math expressions can contain function calls, and function args
// can be math expressions)
void generate_expression(ASTNode node);

// HELPER FUNCTIONS
// ----------------
// prints string in ifjcode26 format (like PUSHS string@hello\032world)
// ifjcode26 cant have raw spaces, newlines, '#' or '\' inside strings,
// so we have to convert them to \XYZ decimal ascii codes
void print_escaped_string(char *str) {
  printf("PUSHS string@");
  if (str != NULL) {
    for (int i = 0; str[i] != '\0'; i++) {
      // cast to unsigned char so special/czech chars arent negative
      unsigned char c = str[i];
      if (c <= 32 || c == 35 || c == 92) {
        printf("\\%03d", c);
      } else {
        printf("%c", c);
      }
    }
  }
  printf("\n");
}

// BUILTIN FUNCTIONS
// -----------------
// instead of generating normal function CALL and setting up call frames,
// we print the ifjcode26 instructions for builtin functions directly in place
void generate_builtin(ASTNode node) {
  // safety check: according to AST reference, NODE_FUNC_CALL always has 2
  // children child_arr[0] = identifier with function name, child_arr[1] = arg
  // list with parameters
  if (node == NULL || node->child_count < 2) {
    return;
  }

  // get function name from first child and argument list from second child
  char *func_name = node->child_arr[0]->data.string_val;
  ASTNode args = node->child_arr[1];

  // avoid segfault if pointers are missing
  if (func_name == NULL || args == NULL) {
    return;
  }

  // print(arg1, arg2, ...) - can have any number of arguments separated by
  // commas
  if (strcmp(func_name, "print") == 0) {
    // loop thru all arguments inside arg_list from left to right
    for (int i = 0; i < args->child_count; i++) {
      // compute the argument and leave its value on top of the data stack
      generate_expression(args->child_arr[i]);
      // WRITE in ifjcode26 cant read straight from the stack, so we have to
      // pop the result into our global helper variable first and then write it
      printf("POPS GF@temp_result\n");
      printf("WRITE GF@temp_result\n");
    }
    // print returns void/Unit, but every function call in an expression must
    // leave 1 return value on the stack, so we push nil (if print is alone on a
    // line, generate_statement will immediately pop and throw away this nil)
    printf("PUSHS nil@nil\n");
  }
  // readInt() - reads integer from user input into temp_result and pushes it on
  // stack
  else if (strcmp(func_name, "readInt") == 0) {
    printf("READ GF@temp_result int\n");
    printf("PUSHS GF@temp_result\n");
  }
  // readDouble() - reads float from user input into temp_result and pushes it
  // on stack
  else if (strcmp(func_name, "readDouble") == 0) {
    printf("READ GF@temp_result float\n");
    printf("PUSHS GF@temp_result\n");
  }
  // readLine() - reads string line from user input into temp_result and pushes
  // it on stack
  else if (strcmp(func_name, "readLine") == 0) {
    printf("READ GF@temp_result string\n");
    printf("PUSHS GF@temp_result\n");
  }
  // strLength(s) - counts string length
  else if (strcmp(func_name, "strLength") == 0) {
    if (args->child_count > 0) {
      // evaluate first argument onto stack
      generate_expression(args->child_arr[0]);
      // STRLEN only works with variables (not stack), so pop -> calculate ->
      // push result back
      printf("POPS GF@temp_result\n");
      printf("STRLEN GF@temp_result GF@temp_result\n");
      printf("PUSHS GF@temp_result\n");
    }
  }
  // toInt(d) - converts float number to int (cuts off decimal part)
  else if (strcmp(func_name, "toInt") == 0) {
    if (args->child_count > 0) {
      generate_expression(args->child_arr[0]);
      // FLOAT2INT also needs variable, so pop to temp_result, convert and push
      // back
      printf("POPS GF@temp_result\n");
      printf("FLOAT2INT GF@temp_result GF@temp_result\n");
      printf("PUSHS GF@temp_result\n");
    }
  }
  // chr(i) - converts ascii integer number into a 1-char string
  else if (strcmp(func_name, "chr") == 0) {
    if (args->child_count > 0) {
      generate_expression(args->child_arr[0]);
      // pop ascii number, convert with INT2CHAR and push string back on stack
      printf("POPS GF@temp_result\n");
      printf("INT2CHAR GF@temp_result GF@temp_result\n");
      printf("PUSHS GF@temp_result\n");
    }
  }
}

// EXPRESSIONS AND STACK MATH
// --------------------------
// calculates expressions bottom-up using the ifjcode26 data stack
// first we push operands (numbers and variables) on stack, and then stack
// instructions (like ADDS or LTS) take top 2 values, compute and push result
// back
void generate_expression(ASTNode node) {
  if (node == NULL) {
    return;
  }

  // we must check function call FIRST before going into children below!
  // because NODE_FUNC_CALL has 2 children (name and arg list), letting it
  // continue to child_count == 2 would treat it like binary math and evaluate
  // args twice
  if (node->node_type == NODE_FUNC_CALL) {
    // check symtable via child 0 (identifier) to see if its a builtin function
    if (node->child_count > 0 && node->child_arr[0]->symbol_ptr != NULL) {
      if (node->child_arr[0]->symbol_ptr->symb_type == SB_BUILTIN) {
        generate_builtin(node);
      }
    }
    // stop here because generate_builtin already handled all children
    return;
  }

  // go to children first so operands are ready on stack before we do the
  // operation
  if (node->child_count == 2) {
    // binary op: evaluate left side first (index 0), then right side (index 1)
    generate_expression(node->child_arr[0]);
    generate_expression(node->child_arr[1]);
  }
  // unary op (like -x) or wrapper nodes (NODE_EXPR / NODE_COND from reference)
  // wrapper nodes have 1 child so this just passes straight down to the actual
  // math
  else if (node->child_count == 1) {
    generate_expression(node->child_arr[0]);
  }

  // push literal constant values on stack based on their data type
  if (node->node_type == NODE_LITERAL) {
    if (node->data_type == DT_INTEGER) {
      printf("PUSHS int@%d\n", node->data.int_val);
    } else if (node->data_type == DT_DOUBLE) {
      // %a prints double in hex format (like 0x1.8p+1) which ifjcode26 requires
      printf("PUSHS float@%a\n", node->data.double_val);
    } else if (node->data_type == DT_STRING) {
      // strings need ascii escape codes for spaces and special chars
      print_escaped_string(node->data.string_val);
    } else if (node->data_type == DT_BOOL) {
      if (node->data.int_val != 0) {
        printf("PUSHS bool@true\n");
      } else {
        printf("PUSHS bool@false\n");
      }
    }
  }
  // push variable value from local frame (LF) onto stack
  // we use unique symbol_ptr->id instead of variable name so variables with
  // the same name in different blocks dont overwrite each other
  else if (node->node_type == NODE_IDENTIFIER) {
    if (node->symbol_ptr != NULL) {
      printf("PUSHS LF@var_%d\n", node->symbol_ptr->id);
    }
  }
  // addition or string concatenation
  else if (node->node_type == NODE_ADD) {
    if (node->data_type == DT_STRING) {
      // ifjcode26 has no stack CONCATS, so pop both strings into helper vars
      // (right operand is on top of stack, so pop op2 first!)
      printf("POPS GF@temp_op2\n");
      printf("POPS GF@temp_op1\n");
      printf("CONCAT GF@temp_result GF@temp_op1 GF@temp_op2\n");
      printf("PUSHS GF@temp_result\n");
    } else {
      printf("ADDS\n");
    }
  } else if (node->node_type == NODE_SUB) {
    printf("SUBS\n");
  } else if (node->node_type == NODE_MUL) {
    printf("MULS\n");
  } else if (node->node_type == NODE_DIV) {
    // DIVS for float division, IDIVS for integer division
    if (node->data_type == DT_DOUBLE) {
      printf("DIVS\n");
    } else {
      printf("IDIVS\n");
    }
  } else if (node->node_type == NODE_UN_PLUS) {
    // unary plus (+x) doesnt change anything, child is already on stack
  } else if (node->node_type == NODE_UN_MINUS) {
    // multiply by minus one trick because ifjcode26 has no stack negate
    // instruction
    printf("PUSHS int@-1\n");
    printf("MULS\n");
  }
  // implicit conversion from int to double on stack (child already pushed int)
  else if (node->node_type == NODE_INT_TO_DBL) {
    printf("INT2FLOATS\n");
  }
  // relacional operators for if/while conditons (push bool@true or bool@false
  // on stack)
  else if (node->node_type == NODE_EQ) {
    printf("EQS\n");
  } else if (node->node_type == NODE_NOT_EQ) {
    // ifjcode26 has no != instruction, so we check == and negate it with NOTS
    printf("EQS\n");
    printf("NOTS\n");
  } else if (node->node_type == NODE_LT) {
    printf("LTS\n");
  } else if (node->node_type == NODE_GT) {
    printf("GTS\n");
  } else if (node->node_type == NODE_LT_EQ) {
    // <= is the same as "not greater" (!(a > b))
    printf("GTS\n");
    printf("NOTS\n");
  } else if (node->node_type == NODE_GT_EQ) {
    // >= is the same as "not less" (!(a < b))
    printf("LTS\n");
    printf("NOTS\n");
  }
}

// STATEMENTS AND CONTROL FLOW
// ---------------------------
// handles individual commands (var definitions, assignments, if/while, blocks)
void generate_statement(ASTNode node) {
  if (node == NULL) {
    return;
  }

  // standalone function call on its own line (like print("hi"))
  if (node->node_type == NODE_FUNC_CALL) {
    generate_expression(node);
    // every function leaves a return value on stack, so throw it away into
    // temp_result so the stack doesnt fill up with unused values
    printf("POPS GF@temp_result\n");
  }
  // new variable definition (var x = ... or val x = ...)
  else if (node->node_type == NODE_VAR_DEF || node->node_type == NODE_VAL_DEF) {
    // in AST reference: child_arr[0] is identifier (with id), child_arr[1] is
    // value expression
    if (node->child_count == 2 && node->child_arr[0]->symbol_ptr != NULL) {
      // declare variable in local frame, compute initial value, then pop it
      // into the var
      printf("DEFVAR LF@var_%d\n", node->child_arr[0]->symbol_ptr->id);
      generate_expression(node->child_arr[1]);
      printf("POPS LF@var_%d\n", node->child_arr[0]->symbol_ptr->id);
    }
  }
  // assignment to an already existing variable (x = ...)
  else if (node->node_type == NODE_ASSIGNMENT) {
    // same child layout as VAR_DEF, but NO DEFVAR because variable already
    // exists (calling DEFVAR twice on the same variable causes a runtime error)
    if (node->child_count == 2 && node->child_arr[0]->symbol_ptr != NULL) {
      generate_expression(node->child_arr[1]);
      printf("POPS LF@var_%d\n", node->child_arr[0]->symbol_ptr->id);
    }
  }
  // if-else statment: child 0 is conditon, child 1 is then block, child 2 is
  // optional else block
  else if (node->node_type == NODE_IF) {
    if (node->child_count >= 2) {
      // reserve unique label number for this if block
      int lbl = label_id++;
      // evaluate conditon (leaves bool@true or bool@false on stack)
      generate_expression(node->child_arr[0]);
      printf("PUSHS bool@false\n");
      // if conditon equals false, jump straight to else branch
      printf("JUMPIFEQS $else_%d\n", lbl);

      // conditon was true -> run then brach and jump over else to end
      generate_statement(node->child_arr[1]);
      printf("JUMP $end_if_%d\n", lbl);

      // else branch target (runs only when conditon was false)
      printf("LABEL $else_%d\n", lbl);
      if (node->child_count == 3) {
        generate_statement(node->child_arr[2]);
      }
      printf("LABEL $end_if_%d\n", lbl);
    }
  }
  // while loop: child 0 is conditon, child 1 is loop body block
  else if (node->node_type == NODE_WHILE) {
    if (node->child_count == 2) {
      int lbl = label_id++;
      // put start label before conditon so we can jump back and re-check every
      // loop
      printf("LABEL $while_start_%d\n", lbl);
      // check conditon, if it equals false jump out of loop to end label
      generate_expression(node->child_arr[0]);
      printf("PUSHS bool@false\n");
      printf("JUMPIFEQS $while_end_%d\n", lbl);

      // conditon was true -> run loop body and jump back to start
      generate_statement(node->child_arr[1]);
      printf("JUMP $while_start_%d\n", lbl);
      printf("LABEL $while_end_%d\n", lbl);
    }
  }
  // container nodes (NODE_PROGRAM, NODE_MAIN_FUNC_DEF, NODE_BLOCK, etc.)
  else {
    // just loop thru all children in order from left to right
    for (int i = 0; i < node->child_count; i++) {
      generate_statement(node->child_arr[i]);
    }
  }
}

// MAIN CODE GENERATOR
// -------------------
// entry point called from main.c after parser and semantic analysis finish
void generate_code(ASTNode root) {
  // mandatory header required at the very top of every ifjcode26 output
  printf(".IFJcode26\n");

  // global helper variables (GF = Global Frame, accessible from anywhere)
  // used when instructions cant work directly with stack (like WRITE, STRLEN,
  // CONCAT) and for throwing away unused function return values
  printf("DEFVAR GF@temp_result\n");
  printf("DEFVAR GF@temp_op1\n");
  printf("DEFVAR GF@temp_op2\n");

  // temp frame setup: create a frame and push it so LF (Local Frame) variables
  // work for testing before we implement user functions (CALL and RETURN)
  printf("CREATEFRAME\n");
  printf("PUSHFRAME\n");

  // start traversing the AST from root
  if (root != NULL) {
    generate_statement(root);
  }
}