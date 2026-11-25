// Project: prekladac-ifj26
// File: codegen.c
// Author: Lukáš Dvořák (xdvoral00)

/* AI usage: Gemini was used to consult IFJcode26 string escaping, DEFVAR
   hoisting, user function stack frames, control flow (if, while, return), and
   to generate most of the comments in this file becouse i suck in makeing good
   comments but the "how this file works" and marknig of key fitures was my
   idea. I also found some bugs in the code (mainly in substring temp
   variables, stack and bounds) which AI helped me fix. */

/* how this file works (in order of calls):
1. generate_code() - prints header, temp vars and jumps to $main
2. generate_function() - creates frame, declares vars, pops params and runs
   body
3. generate_statement() - runs commands (var/val, =, if, while, return,
   blocks)
4. generate_expression() - computes math, strings, func calls and comparisons
   on stack
5. generate_builtin() - prints code for builtin functions directly (print,
   read...)
6. helper functions - print_escaped_string() and declare_local_vars() */

#include "codegen.h"
#include <stdio.h>
#include <string.h> // needed for strcmp when checking builtin function names

// GLOBAL VARIABLES AND DECLARATIONS
// ---------------------------------

// counter for unik labels so if and while jumps dont get the same name
// if two ifs had the same label name, the ifjcode26 interpreter would crash
int label_id = 0;

// flag to know if we are currently inside main() function
// main uses EXIT instead of RETURN because we jump to $main without CALL
int is_main_func = 0;

// forward declarations because functions call each other recursively
void generate_expression(ASTNode node);
void generate_statement(ASTNode node);

/* HELPER FUNCTIONS
   ----------------
   prints string in ifjcode26 format (like PUSHS string@hello\032world)
   ifjcode26 cant have raw spaces, newlines, '#' or '\' inside strings,
   so we have to convert them to \XYZ decimal ascii codes
   (AI-assisted, prompt: "potřebuju v C napsat funkci, která mi vypíše string ve
   formátu pro ifjcode26 a všechny mezery, odřádkování, křížky a lomítka v něm
   převede na dekadický ascii kód \XYZ, protože to interpret jinak nevezme") */
void print_escaped_string(char *str) {
  printf("PUSHS string@");
  if (str != NULL) {
    // go char by char until null terminator
    for (int i = 0; str[i] != '\0'; i++) {
      // cast to unsigned char so special/czech chars arent negative
      unsigned char c = str[i];
      // ascii 0-32 are control chars + space, 35 is '#', 92 is '\'
      if (c <= 32 || c == 35 || c == 92) {
        printf("\\%03d", c);
      } else {
        printf("%c", c);
      }
    }
  }
  printf("\n");
}

/* scans the whole function body before running it and prints DEFVAR for every
   local variable at the top of the function
   we have to do this because ifjcode26 crashes if DEFVAR runs twice inside a
   while loop
   (AI-assisted, prompt: "interpret ifjcode26 mi padá na redefinici proměnné
   když udělám DEFVAR uvnitř while cyklu a proběhne to podruhé, jak mám projít
   AST strom celé funkce dopředu a vypsat všechny DEFVAR hned na začátek
   funkce?") */
void declare_local_vars(ASTNode node) {
  if (node == NULL) {
    return;
  }

  // if we found var or val definition, declare it in local frame right now
  if (node->node_type == NODE_VAR_DEF || node->node_type == NODE_VAL_DEF) {
    if (node->child_count >= 1 && node->child_arr[0] != NULL &&
        node->child_arr[0]->symbol_ptr != NULL) {
      printf("DEFVAR LF@var_%d\n", node->child_arr[0]->symbol_ptr->uniq_id);
    }
  }

  // keep searching recursively in all children (blocks, ifs, whiles...)
  for (int i = 0; i < node->child_count; i++) {
    declare_local_vars(node->child_arr[i]);
  }
}

/* BUILTIN FUNCTIONS
   -----------------
   instead of generating normal function CALL and setting up call frames,
   we print the ifjcode26 instructions for builtin functions directly in place
 */
void generate_builtin(ASTNode node) {
  /* safety check: according to AST reference, NODE_FUNC_CALL always has 2
     children child_arr[0] = identifier with function name, child_arr[1] = arg
     list with parameters */
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
    /* print returns void/Unit, but every function call in an expression must
       leave 1 return value on the stack, so we push nil (if print is alone on a
       line, generate_statement will immediately pop and throw away this nil) */
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
  // ord(s, i) - gets ascii code of char at index i
  // (AI-assisted, prompt: "jak v ifjcode26 vytáhnout ascii hodnotu znaku na
  // indexu i ze stringu, když mám obojí na zásobníku?")
  else if (strcmp(func_name, "ord") == 0) {
    if (args->child_count >= 2) {
      // evaluate string (arg 0) and index (arg 1) onto stack
      generate_expression(args->child_arr[0]);
      generate_expression(args->child_arr[1]);
      // index is on top so pop it to temp_op2 first, string goes to temp_op1
      printf("POPS GF@temp_op2\n");
      printf("POPS GF@temp_op1\n");
      // STRI2INT reads char from temp_op1 at position temp_op2 as int
      printf("STRI2INT GF@temp_result GF@temp_op1 GF@temp_op2\n");
      printf("PUSHS GF@temp_result\n");
    }
  }
  // strcmp(a, b) - compares two strings, returns -1, 0 or 1
  // (AI-assisted, prompt: "jak mám v ifjcode26 porovnat dva stringy ze
  // zásobníku a vrátit -1, 0 nebo 1 podle toho jestli je první menší, stejný
  // nebo větší, když na to není jedna instrukce?")
  else if (strcmp(func_name, "strcmp") == 0) {
    if (args->child_count >= 2) {
      // unique label id so multiple strcmp calls in one program dont collide
      int lbl = label_id++;
      // compute both strings onto stack and pop them into helper vars
      generate_expression(args->child_arr[0]);
      generate_expression(args->child_arr[1]);
      printf("POPS GF@temp_op2\n");
      printf("POPS GF@temp_op1\n");

      // check if strings are equal first -> if yes jump straight to 0
      printf("JUMPIFEQ $strcmp_eq_%d GF@temp_op1 GF@temp_op2\n", lbl);
      // not equal -> push them back on stack to test if op1 < op2
      printf("PUSHS GF@temp_op1\n");
      printf("PUSHS GF@temp_op2\n");
      printf("LTS\n");
      printf("PUSHS bool@true\n");
      printf("JUMPIFEQS $strcmp_lt_%d\n", lbl);

      // otherwise op1 > op2 -> push 1 and skip the other branches
      printf("PUSHS int@1\n");
      printf("JUMP $strcmp_end_%d\n", lbl);

      // op1 was smaller than op2 -> push -1
      printf("LABEL $strcmp_lt_%d\n", lbl);
      printf("PUSHS int@-1\n");
      printf("JUMP $strcmp_end_%d\n", lbl);

      // strings were identical -> push 0
      printf("LABEL $strcmp_eq_%d\n", lbl);
      printf("PUSHS int@0\n");

      printf("LABEL $strcmp_end_%d\n", lbl);
    }
  }
  // substring(s, start, end) - returns substring from start to end, or nil on
  // invalid bounds (bugs in first version fixed with AI help)
  // (AI-assisted, prompt: "jak v ifjcode26 napsat funkci substring(s, start,
  // end) včetně kontroly mezí, aby při neplatných indexech vrátila nil a v
  // cyklu přes GETCHAR a CONCAT nerozhodila zásobník?")
  else if (strcmp(func_name, "substring") == 0) {
    if (args->child_count >= 3) {
      int lbl = label_id++;
      // evaluate all 3 arguments onto stack: string, start, end
      generate_expression(args->child_arr[0]);
      generate_expression(args->child_arr[1]);
      generate_expression(args->child_arr[2]);

      // pop arguments in reverse order: end, start, string
      printf("POPS GF@temp_op2\n");    // end index
      printf("POPS GF@temp_op1\n");    // start index (also loop counter)
      printf("POPS GF@temp_result\n"); // source string

      // check invalid bounds (start < 0, end < 0, start > end, start >= len,
      // end > len)
      printf("STRLEN GF@temp_len GF@temp_result\n");

      printf("LT GF@temp_cmp GF@temp_op1 int@0\n");
      printf("JUMPIFEQ $sub_nil_%d GF@temp_cmp bool@true\n", lbl);

      printf("LT GF@temp_cmp GF@temp_op2 int@0\n");
      printf("JUMPIFEQ $sub_nil_%d GF@temp_cmp bool@true\n", lbl);

      printf("GT GF@temp_cmp GF@temp_op1 GF@temp_op2\n");
      printf("JUMPIFEQ $sub_nil_%d GF@temp_cmp bool@true\n", lbl);

      printf("LT GF@temp_cmp GF@temp_op1 GF@temp_len\n");
      printf("JUMPIFEQ $sub_nil_%d GF@temp_cmp bool@false\n", lbl);

      printf("GT GF@temp_cmp GF@temp_op2 GF@temp_len\n");
      printf("JUMPIFEQ $sub_nil_%d GF@temp_cmp bool@true\n", lbl);

      // build substring char by char without touching the stack inside the
      // loop
      printf("MOVE GF@temp_out string@\n");
      printf("LABEL $sub_loop_%d\n", lbl);
      printf("LT GF@temp_cmp GF@temp_op1 GF@temp_op2\n");
      printf("JUMPIFEQ $sub_done_%d GF@temp_cmp bool@false\n", lbl);

      // read single char at current index and append it to output string
      printf("GETCHAR GF@temp_ch GF@temp_result GF@temp_op1\n");
      printf("CONCAT GF@temp_out GF@temp_out GF@temp_ch\n");
      printf("ADD GF@temp_op1 GF@temp_op1 int@1\n");
      printf("JUMP $sub_loop_%d\n", lbl);

      // valid bounds -> push built substring on stack and skip nil branch
      printf("LABEL $sub_done_%d\n", lbl);
      printf("PUSHS GF@temp_out\n");
      printf("JUMP $sub_end_%d\n", lbl);

      // invalid bounds -> push nil@nil on stack
      printf("LABEL $sub_nil_%d\n", lbl);
      printf("PUSHS nil@nil\n");

      printf("LABEL $sub_end_%d\n", lbl);
    }
  }
}

/* EXPRESSIONS AND STACK MATH
   --------------------------
   calculates expressions bottom-up using the ifjcode26 data stack
   first we push operands (numbers and variables) on stack, and then stack
   instructions (like ADDS or LTS) take top 2 values, compute and push result
   back */
void generate_expression(ASTNode node) {
  if (node == NULL) {
    return;
  }

  /* we must check function call FIRST before going into children below!
     because NODE_FUNC_CALL has 2 children (name and arg list), letting it
     continue to child_count == 2 would treat it like binary math and evaluate
     args twice */
  if (node->node_type == NODE_FUNC_CALL) {
    if (node->child_count >= 2 && node->child_arr[0] != NULL) {
      // check symtable via child 0 (identifier) to see if its a builtin
      // function
      if (node->child_arr[0]->symbol_ptr != NULL &&
          node->child_arr[0]->symbol_ptr->symb_type == SB_BUILTIN) {
        generate_builtin(node);
      } else {
        // user defined function call:
        // 1. push all arguments onto stack from left to right
        ASTNode args = node->child_arr[1];
        if (args != NULL) {
          for (int i = 0; i < args->child_count; i++) {
            generate_expression(args->child_arr[i]);
          }
        }
        // 2. prepare empty frame for the function and jump to its label
        char *func_name = node->child_arr[0]->data.string_val;
        if (func_name != NULL) {
          printf("CREATEFRAME\n");
          printf("CALL $%s\n", func_name);
        }
      }
    }
    // stop here because we already handled all children
    return;
  }

  // go to children first so operands are ready on stack before we do the
  // operation
  if (node->child_count == 2) {
    // binary op: evaluate left side first (index 0), then right side (index 1)
    generate_expression(node->child_arr[0]);
    generate_expression(node->child_arr[1]);
  }
  /* unary op (like -x) or wrapper nodes (NODE_EXPR / NODE_COND from reference)
     wrapper nodes have 1 child so this just passes straight down to the actual
     math */
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
      // ifjcode26 expects bool@true or bool@false instead of 1/0
      if (node->data.bool_val) {
        printf("PUSHS bool@true\n");
      } else {
        printf("PUSHS bool@false\n");
      }
    }
  }
  /* push variable value from local frame (LF) onto stack
     we use unique symbol_ptr->uniq_id instead of variable name so variables
     with the same name in different blocks dont overwrite each other */
  else if (node->node_type == NODE_IDENTIFIER) {
    if (node->symbol_ptr != NULL) {
      printf("PUSHS LF@var_%d\n", node->symbol_ptr->uniq_id);
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
    // subtracts top stack value from second stack value
    printf("SUBS\n");
  } else if (node->node_type == NODE_MUL) {
    // multiplies top 2 values on stack
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
    // instruction (must match operand type, otherwise interpreter crashes)
    if (node->data_type == DT_DOUBLE) {
      printf("PUSHS float@-0x1p+0\n");
    } else {
      printf("PUSHS int@-1\n");
    }
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
    // checks if left operand < right operand
    printf("LTS\n");
  } else if (node->node_type == NODE_GT) {
    // checks if left operand > right operand
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

/* USER FUNCTIONS
   --------------
   generates code for a function definition (both main and normal functions)
   child 0 = function identifier, child 1 = param list, child 2 = body
   (AI-assisted, prompt: "jak mám v ifjcode26 řešit volání uživatelských
   funkcí a hlavní funkce main, když main se nevolá přes CALL ale skáče se na
   něj přes JUMP, a v jakém pořadí mám ze zásobníku vybírat parametry funkce?")
 */
void generate_function(ASTNode node) {
  // safety check: function definition must have id, params and body
  if (node == NULL || node->child_count < 3) {
    return;
  }

  // get function name node, parameter list node and body node
  ASTNode func_id = node->child_arr[0];
  ASTNode params = node->child_arr[1];
  ASTNode body = node->child_arr[2];

  if (func_id == NULL || func_id->data.string_val == NULL || body == NULL) {
    return;
  }

  // check if this is the main function
  if (node->node_type == NODE_MAIN_FUNC_DEF) {
    is_main_func = 1;
    printf("\nLABEL $main\n");
    // main wasnt called via CALL, so it has to create its own frame first
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
  } else {
    is_main_func = 0;
    printf("\nLABEL $%s\n", func_id->data.string_val);
    // caller already did CREATEFRAME before CALL, so just push it to LF
    printf("PUSHFRAME\n");
  }

  // declare all local variables at the top of the function before loops run
  declare_local_vars(body);

  // pop parameters from stack in reverse order (last arg is on top of stack!)
  if (params != NULL) {
    for (int i = params->child_count - 1; i >= 0; i--) {
      ASTNode param = params->child_arr[i];
      if (param != NULL && param->symbol_ptr != NULL) {
        // define parameter as local variable and pop its value from stack
        printf("DEFVAR LF@var_%d\n", param->symbol_ptr->uniq_id);
        printf("POPS LF@var_%d\n", param->symbol_ptr->uniq_id);
      }
    }
  }

  // function body can be a normal block { ... } or a single expression (= expr)
  if (body->node_type == NODE_BLOCK) {
    generate_statement(body);
  } else {
    // expression body (like fun f() = 5): compute value onto stack and return
    // it
    generate_expression(body);
    if (is_main_func == 1) {
      // clean up frame and exit program if this was main
      printf("POPFRAME\n");
      printf("EXIT int@0\n");
    } else {
      // clean up frame and return to caller with result on stack
      printf("POPFRAME\n");
      printf("RETURN\n");
    }
    return;
  }

  // fallback end of function if it didnt hit a return statement
  if (is_main_func == 1) {
    printf("POPFRAME\n");
    printf("EXIT int@0\n");
  } else {
    // push nil@nil so caller always gets 1 return value on stack to pop
    printf("PUSHS nil@nil\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
  }
}

/* STATEMENTS AND CONTROL FLOW
   ---------------------------
   handles individual commands (var definitions, assignments, if/while, blocks)
 */
void generate_statement(ASTNode node) {
  if (node == NULL) {
    return;
  }

  // function definitions (NODE_MAIN_FUNC_DEF or NODE_FUNC_DEF)
  if (node->node_type == NODE_MAIN_FUNC_DEF ||
      node->node_type == NODE_FUNC_DEF) {
    generate_function(node);
  }
  // return statement: evaluate return expression onto stack, clean frame and
  // return
  // (AI-assisted, prompt: "jak mám v ifjcode26 ošetřit příkaz return, když se
  // může objevit jak v normální funkci, tak i v mainu kde se místo RETURN musí
  // zavolat EXIT, a co dát na zásobník když je return prázdný bez hodnoty?")
  else if (node->node_type == NODE_RETURN) {
    if (node->child_count > 0 && node->child_arr[0] != NULL) {
      // return with expression (like return x + 1) -> leave result on stack
      generate_expression(node->child_arr[0]);
    } else {
      // empty return without value -> push nil so stack stays balanced
      printf("PUSHS nil@nil\n");
    }

    if (is_main_func == 1) {
      // returning from main ends the whole program with exit code 0
      printf("POPFRAME\n");
      printf("EXIT int@0\n");
    } else {
      // returning from normal function pops frame and jumps back after CALL
      printf("POPFRAME\n");
      printf("RETURN\n");
    }
  }
  // standalone function call on its own line (like print("hi") or foo(1))
  else if (node->node_type == NODE_FUNC_CALL) {
    generate_expression(node);
    // every function leaves a return value on stack, so throw it away into
    // temp_result so the stack doesnt fill up with unused values
    printf("POPS GF@temp_result\n");
  }
  // new variable definition (var x = ... or val x = ...)
  else if (node->node_type == NODE_VAR_DEF || node->node_type == NODE_VAL_DEF) {
    /* NOTE: DEFVAR was already printed at the top of the function by
       declare_local_vars() so here we just compute the initial value and pop it
       into the variable */
    if (node->child_count == 2 && node->child_arr[0]->symbol_ptr != NULL) {
      generate_expression(node->child_arr[1]);
      printf("POPS LF@var_%d\n", node->child_arr[0]->symbol_ptr->uniq_id);
    }
  }
  // assignment to an already existing variable (x = ...)
  else if (node->node_type == NODE_ASSIGNMENT) {
    /* same child layout as VAR_DEF, but NO DEFVAR because variable already
       exists (calling DEFVAR twice on the same variable causes a runtime error)
     */
    if (node->child_count == 2 && node->child_arr[0]->symbol_ptr != NULL) {
      generate_expression(node->child_arr[1]);
      printf("POPS LF@var_%d\n", node->child_arr[0]->symbol_ptr->uniq_id);
    }
  }
  // if-else statment: child 0 is conditon, child 1 is then block, child 2 is
  // optional else block
  // (AI-assisted, prompt: "jak v ifjcode26 přeložit podmínku if-else přes
  // zásobníkové instrukce JUMPIFEQS, a jak ošetřit návěští aby se mi při
  // zanořených ifech nepřepisovala?")
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
  // (AI-assisted, prompt: "jak v ifjcode26 napsat cyklus while s vyhodnocením
  // podmínky na zásobníku přes JUMPIFEQS a unikátními návěštími pro začátek a
  // konec cyklu?")
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
  // container nodes (NODE_PROGRAM, NODE_BLOCK, etc.)
  else {
    // just loop thru all children in order from left to right
    for (int i = 0; i < node->child_count; i++) {
      generate_statement(node->child_arr[i]);
    }
  }
}

/* MAIN CODE GENERATOR
   -------------------
   entry point called from main.c after parser and semantic analysis finish */
void generate_code(ASTNode root) {
  // mandatory header required at the very top of every ifjcode26 output
  printf(".IFJcode26\n");

  /* global helper variables (GF = Global Frame, accessible from anywhere)
     used when instructions cant work directly with stack (like WRITE, STRLEN,
     CONCAT) and for throwing away unused function return values */
  printf("DEFVAR GF@temp_result\n");
  printf("DEFVAR GF@temp_op1\n");
  printf("DEFVAR GF@temp_op2\n");
  // extra helper vars needed for substring loop and bounds checks
  printf("DEFVAR GF@temp_out\n");
  printf("DEFVAR GF@temp_ch\n");
  printf("DEFVAR GF@temp_len\n");
  printf("DEFVAR GF@temp_cmp\n");

  // jump straight to $main so other functions defined above main dont run first
  printf("JUMP $main\n");

  // start traversing the AST from root
  if (root != NULL) {
    generate_statement(root);
  }
}