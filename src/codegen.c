// Project: prekladac-ifj26
// File: codegen.c
// Author: Lukas Dvorak (xdvoral00)

/* AI usage: Gemini was used to generate IFJcode26 string escaping, DEFVAR
   hoisting, user function stack frames, control flow (if, while, return) and
   checked or adjusted by me, and to generate most of the comments in this file
   becouse i suck in makeing good comments but the "how this file works" and
   marknig of key fitures was my idea. I also found some bugs in the code (in
   subStr, ord, readInt/readDouble/readLine, missing toStr, implicit function
   return value, printing Unit, Unit function return discarding, $$entry_main
   label collision and standalone expression statements) which AI helped me
   fix, and removed an unused strcmp builtin that AI mistakenly suggested from
   older IFJ specs. */

/* how this file works (in order of calls):
1. generate_code() - prints header, temp vars and jumps to $$entry_main
2. generate_function() - creates frame, declares vars, pops params and runs
   body
3. generate_statement() - runs commands (var/val, =, if, while, return,
   blocks, standalone expressions)
4. generate_expression() - computes math, strings, func calls and comparisons
   on stack
5. generate_builtin() - prints code for builtin functions directly (print,
   read...)
6. helper functions - print_escaped_string(), declare_local_vars() and
   is_value_node() */

#include "codegen.h"
#include "ast_module.h"
#include "symtable.h"
#include <stdio.h>
#include <string.h> // needed for strcmp when checking builtin function names

// GLOBAL VARIABLES AND DECLARATIONS
// ---------------------------------

// counter for unik labels so if and while jumps dont get the same name
// if two ifs had the same label name, the ifjcode26 interpreter would crash
int label_id = 0;

// flag to know if the current function has explicit Unit return type
// Unit functions must evaluate return expressions, discard them and return ()
int is_unit_func = 0;

// forward declarations because functions call each other recursively
void generate_expression(ASTNode node);
void generate_statement(ASTNode node);

/* HELPER FUNCTIONS
   ----------------
   prints string in ifjcode26 format (like PUSHS string@hello\032world)
   ifjcode26 cant have raw spaces, newlines, '#' or '\' inside strings,
   so we have to convert them to \XYZ decimal ascii codes
   (AI-assisted, prompt: "potrebuju v C napsat funkci, ktera mi vypise string ve
   formatu pro ifjcode26 a vsechny mezery, odradkovani, krizky a lomitka v nem
   prevede na dekadicky ascii kod \XYZ, protoze to interpret jinak nevezme") */
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
   (AI-assisted, prompt: "interpret ifjcode26 mi pada na redefinici promenne
   kdyz udelam DEFVAR uvnitr while cyklu a probehne to podruhe, jak mam projit
   AST strom cele funkce dopredu a vypsat vsechny DEFVAR hned na zacatek
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

/* checks if an AST node is an expression that produces a value on the stack
   used at the end of a function block when there is no explicit return keyword
   and for standalone expression statements in blocks
   (AI-assisted, prompt: "nasel jsem chybu ze kdyz funkce konci vyrazem bez
   klicoveho slova return, tak mi misto vysledku vrati nil, jak to v
   generate_function opravit?") */
int is_value_node(ASTNode node) {
  if (node == NULL) {
    return 0;
  }
  switch (node->node_type) {
  case NODE_VAR_DEF:
  case NODE_VAL_DEF:
  case NODE_ASSIGNMENT:
  case NODE_IF:
  case NODE_WHILE:
  case NODE_RETURN:
  case NODE_BLOCK:
  case NODE_PROGRAM:
    return 0;
  default:
    return 1;
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

  // print(value) - prints value to output (loops over args if needed)
  // (AI-assisted, prompt: "kdyz vypisuju Unit hodnotu () pres print, tak WRITE
  // nil@nil vypise nil misto (), jak mam v printu zkontrolovat typ pres TYPE a
  // vypsat ()?")
  if (strcmp(func_name, "print") == 0) {
    // loop thru all arguments inside arg_list from left to right
    for (int i = 0; i < args->child_count; i++) {
      int lbl = label_id++;
      // compute the argument and leave its value on top of the data stack
      generate_expression(args->child_arr[i]);
      // WRITE in ifjcode26 cant read straight from the stack, so we have to
      // pop the result into our global helper variable first and then write it
      printf("POPS GF@temp_result\n");
      // Unit is stored as nil@nil, but specification requires printing "()"
      printf("TYPE GF@temp_cmp GF@temp_result\n");
      printf("JUMPIFNEQ $print_ok_%d GF@temp_cmp string@nil\n", lbl);
      printf("MOVE GF@temp_result string@()\n");
      printf("LABEL $print_ok_%d\n", lbl);
      printf("WRITE GF@temp_result\n");
    }
    /* print returns Unit (), but every function call in an expression must
       leave 1 return value on the stack, so we push nil (if print is alone on a
       line, generate_statement will immediately pop and throw away this nil) */
    printf("PUSHS nil@nil\n");
  }
  // readInt() - reads integer from user input, returns 0 on bad input or EOF
  // (AI-assisted, prompt: "instrukce READ v ifjcode26 mi pri chybnem vstupu
  // vraci nil misto 0, jak to mam osetrit aby readInt vratil 0?")
  else if (strcmp(func_name, "readInt") == 0) {
    int lbl = label_id++;
    printf("READ GF@temp_result int\n");
    // if READ failed or hit EOF it puts nil@nil into temp_result
    printf("JUMPIFEQ $read_int_nil_%d GF@temp_result nil@nil\n", lbl);
    printf("PUSHS GF@temp_result\n");
    printf("JUMP $read_int_end_%d\n", lbl);

    printf("LABEL $read_int_nil_%d\n", lbl);
    printf("PUSHS int@0\n");

    printf("LABEL $read_int_end_%d\n", lbl);
  }
  // readDouble() - reads float from user input, returns 0.0 on bad input or EOF
  else if (strcmp(func_name, "readDouble") == 0) {
    int lbl = label_id++;
    printf("READ GF@temp_result float\n");
    // same as readInt, replace nil with 0.0 float
    printf("JUMPIFEQ $read_dbl_nil_%d GF@temp_result nil@nil\n", lbl);
    printf("PUSHS GF@temp_result\n");
    printf("JUMP $read_dbl_end_%d\n", lbl);

    printf("LABEL $read_dbl_nil_%d\n", lbl);
    printf("PUSHS float@0x0p+0\n");

    printf("LABEL $read_dbl_end_%d\n", lbl);
  }
  // readLine() - reads string line from user input, returns "" on EOF
  // (AI-assisted, prompt: "instrukce READ v ifjcode26 vraci pri EOF hodnotu
  // nil misto prazdneho retezce, jak mam v readLine osetrit nil@nil aby
  // funkce vratila prazdny string?")
  else if (strcmp(func_name, "readLine") == 0) {
    int lbl = label_id++;
    printf("READ GF@temp_result string\n");
    // if READ hit EOF it puts nil@nil into temp_result, return "" instead
    printf("JUMPIFEQ $read_line_nil_%d GF@temp_result nil@nil\n", lbl);
    printf("PUSHS GF@temp_result\n");
    printf("JUMP $read_line_end_%d\n", lbl);

    printf("LABEL $read_line_nil_%d\n", lbl);
    printf("PUSHS string@\n");

    printf("LABEL $read_line_end_%d\n", lbl);
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
  // toStr(value) - converts any value (int, float, string, bool, Unit) to
  // string
  // (AI-assisted, prompt: "jak v ifjcode26 napsat funkci toStr(value: Any),
  // ktera za behu zjisti typ hodnoty pres instrukci TYPE a prevede int, float,
  // bool nebo Unit na string?")
  else if (strcmp(func_name, "toStr") == 0) {
    if (args->child_count > 0) {
      int lbl = label_id++;
      generate_expression(args->child_arr[0]);
      printf("POPS GF@temp_result\n");

      // check runtime type of the value in temp_result
      printf("TYPE GF@temp_cmp GF@temp_result\n");
      printf("JUMPIFEQ $tostr_str_%d GF@temp_cmp string@string\n", lbl);
      printf("JUMPIFEQ $tostr_int_%d GF@temp_cmp string@int\n", lbl);
      printf("JUMPIFEQ $tostr_flt_%d GF@temp_cmp string@float\n", lbl);
      printf("JUMPIFEQ $tostr_bool_%d GF@temp_cmp string@bool\n", lbl);

      // fallback for Unit ()
      printf("PUSHS string@()\n");
      printf("JUMP $tostr_end_%d\n", lbl);

      // already a string -> just push it back
      printf("LABEL $tostr_str_%d\n", lbl);
      printf("PUSHS GF@temp_result\n");
      printf("JUMP $tostr_end_%d\n", lbl);

      // int -> convert with INT2STR
      printf("LABEL $tostr_int_%d\n", lbl);
      printf("INT2STR GF@temp_result GF@temp_result\n");
      printf("PUSHS GF@temp_result\n");
      printf("JUMP $tostr_end_%d\n", lbl);

      // float -> convert with FLOAT2STR
      printf("LABEL $tostr_flt_%d\n", lbl);
      printf("FLOAT2STR GF@temp_result GF@temp_result\n");
      printf("PUSHS GF@temp_result\n");
      printf("JUMP $tostr_end_%d\n", lbl);

      // bool -> check if true or false
      printf("LABEL $tostr_bool_%d\n", lbl);
      printf("JUMPIFEQ $tostr_true_%d GF@temp_result bool@true\n", lbl);
      printf("PUSHS string@false\n");
      printf("JUMP $tostr_end_%d\n", lbl);
      printf("LABEL $tostr_true_%d\n", lbl);
      printf("PUSHS string@true\n");

      printf("LABEL $tostr_end_%d\n", lbl);
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
  // ord(s, i) - gets ascii code of char at index i, or -1 if out of bounds
  // (AI-assisted, prompt: "jak v ifjcode26 vytahnout ascii hodnotu znaku na
  // indexu i ze stringu, kdyz mam oboji na zasobniku?")
  else if (strcmp(func_name, "ord") == 0) {
    if (args->child_count >= 2) {
      int lbl = label_id++;
      // evaluate string (arg 0) and index (arg 1) onto stack
      generate_expression(args->child_arr[0]);
      generate_expression(args->child_arr[1]);
      // index is on top so pop it to temp_op2 first, string goes to temp_op1
      printf("POPS GF@temp_op2\n");
      printf("POPS GF@temp_op1\n");

      // check bounds first so STRI2INT doesnt crash with error 58
      printf("STRLEN GF@temp_len GF@temp_op1\n");
      printf("LT GF@temp_cmp GF@temp_op2 int@0\n");
      printf("JUMPIFEQ $ord_err_%d GF@temp_cmp bool@true\n", lbl);
      printf("LT GF@temp_cmp GF@temp_op2 GF@temp_len\n");
      printf("JUMPIFEQ $ord_err_%d GF@temp_cmp bool@false\n", lbl);

      // index is valid -> read char ascii value
      printf("STRI2INT GF@temp_result GF@temp_op1 GF@temp_op2\n");
      printf("PUSHS GF@temp_result\n");
      printf("JUMP $ord_end_%d\n", lbl);

      // empty string or index out of range -> return -1
      printf("LABEL $ord_err_%d\n", lbl);
      printf("PUSHS int@-1\n");

      printf("LABEL $ord_end_%d\n", lbl);
    }
  }
  // NOTE: strcmp was removed because AI confused me with older IFJ specs
  // (IFJ26 compares strings directly via <, >, == operators, not strcmp)
  // subStr(s, start, end) - returns substring from start to end, or exits with
  // error 40 on invalid bounds (fixed with AI help after first version)
  // (AI-assisted, prompt: "jak v ifjcode26 napsat funkci subStr(s, start,
  // end) vcetne kontroly mezi a cyklu pres GETCHAR a CONCAT, a jak ji opravit
  // aby misto nil pri neplatnych indexech ukoncila program pres EXIT 40?")
  else if (strcmp(func_name, "subStr") == 0) {
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
      printf("JUMPIFEQ $sub_err_%d GF@temp_cmp bool@true\n", lbl);

      printf("LT GF@temp_cmp GF@temp_op2 int@0\n");
      printf("JUMPIFEQ $sub_err_%d GF@temp_cmp bool@true\n", lbl);

      printf("GT GF@temp_cmp GF@temp_op1 GF@temp_op2\n");
      printf("JUMPIFEQ $sub_err_%d GF@temp_cmp bool@true\n", lbl);

      printf("LT GF@temp_cmp GF@temp_op1 GF@temp_len\n");
      printf("JUMPIFEQ $sub_err_%d GF@temp_cmp bool@false\n", lbl);

      printf("GT GF@temp_cmp GF@temp_op2 GF@temp_len\n");
      printf("JUMPIFEQ $sub_err_%d GF@temp_cmp bool@true\n", lbl);

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

      // valid bounds -> push built substring on stack and skip error branch
      printf("LABEL $sub_done_%d\n", lbl);
      printf("PUSHS GF@temp_out\n");
      printf("JUMP $sub_end_%d\n", lbl);

      // invalid bounds -> exit program with runtime error 40
      printf("LABEL $sub_err_%d\n", lbl);
      printf("EXIT int@40\n");

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
    } else {
      // Unit / nil literal ()
      printf("PUSHS nil@nil\n");
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
   generates code for a function definition (both @main and normal functions)
   child 0 = function identifier, child 1 = param list, child 2 = body
   (AI-assisted, prompt: "jak mam v ifjcode26 resit volani uzivatelskych
   funkci a hlavni funkce @main bez kolize navesti s beznou funkci main, v
   jakem poradi vybirat parametry ze zasobniku a jak zahodit navratovou hodnotu
   u funkci s explicitnim navratovym typem Unit?") */
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

  // @main entry wrapper calls the actual @main function by its identifier so
  // it never collides with a regular user function named "main"
  if (node->node_type == NODE_MAIN_FUNC_DEF) {
    is_unit_func = 1;
    printf("\nLABEL $$entry_main\n");
    printf("CREATEFRAME\n");
    printf("CALL $%s\n", func_id->data.string_val);
    printf("EXIT int@0\n");
  } else {
    is_unit_func = (func_id->symbol_ptr != NULL &&
                    func_id->symbol_ptr->data_type == DT_UNIT);
  }

  // function label and frame activation (caller already did CREATEFRAME)
  printf("\nLABEL $%s\n", func_id->data.string_val);
  printf("PUSHFRAME\n");

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

  // function body can be an indented block or a single expression (= expr)
  if (body->node_type == NODE_BLOCK) {
    int n = body->child_count;
    for (int i = 0; i < n - 1; i++) {
      generate_statement(body->child_arr[i]);
    }
    // if the last node in the block is an expression without 'return',
    // evaluate it onto stack and return its value implicitly
    ASTNode last = (n > 0) ? body->child_arr[n - 1] : NULL;
    if (last != NULL && is_value_node(last)) {
      generate_expression(last);
      // Unit function evaluates the expression, discards result and returns ()
      if (is_unit_func) {
        printf("POPS GF@temp_result\n");
        printf("PUSHS nil@nil\n");
      }
      printf("POPFRAME\n");
      printf("RETURN\n");
      return;
    }
    generate_statement(last);
  } else {
    // expression body (like def f() = 5): compute value onto stack and return
    // it
    generate_expression(body);
    if (is_unit_func) {
      printf("POPS GF@temp_result\n");
      printf("PUSHS nil@nil\n");
    }
    printf("POPFRAME\n");
    printf("RETURN\n");
    return;
  }

  // fallback end of function if it didnt hit a return statement
  // push nil@nil so caller always gets 1 return value on stack to pop
  printf("PUSHS nil@nil\n");
  printf("POPFRAME\n");
  printf("RETURN\n");
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
  // (AI-assisted, prompt: "jak mam v ifjcode26 osetrit prikaz return u funkce
  // s explicitnim navratovym typem Unit, kde se vyraz musi vyhodnotit ale jeho
  // vysledek zahodit a vratit nil@nil?")
  else if (node->node_type == NODE_RETURN) {
    if (node->child_count > 0 && node->child_arr[0] != NULL) {
      // return with expression (like return x + 1) -> leave result on stack
      generate_expression(node->child_arr[0]);
      if (is_unit_func) {
        printf("POPS GF@temp_result\n");
        printf("PUSHS nil@nil\n");
      }
    } else {
      // empty return without value -> push nil so stack stays balanced
      printf("PUSHS nil@nil\n");
    }

    printf("POPFRAME\n");
    printf("RETURN\n");
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
    if (node->child_count == 2 && node->child_arr[0] != NULL &&
        node->child_arr[0]->symbol_ptr != NULL) {
      generate_expression(node->child_arr[1]);
      printf("POPS LF@var_%d\n", node->child_arr[0]->symbol_ptr->uniq_id);
    }
  }
  // assignment to an already existing variable (x = ...)
  else if (node->node_type == NODE_ASSIGNMENT) {
    /* same child layout as VAR_DEF, but NO DEFVAR because variable already
       exists (calling DEFVAR twice on the same variable causes a runtime error)
     */
    if (node->child_count == 2 && node->child_arr[0] != NULL &&
        node->child_arr[0]->symbol_ptr != NULL) {
      generate_expression(node->child_arr[1]);
      printf("POPS LF@var_%d\n", node->child_arr[0]->symbol_ptr->uniq_id);
    }
  }
  // if-else statment: child 0 is conditon, child 1 is then block, child 2 is
  // optional else block
  // (AI-assisted, prompt: "jak v ifjcode26 prelozit podminku if-else pres
  // zasobnikove instrukce JUMPIFEQS, a jak osetrit navesti aby se mi pri
  // zanorenych ifech neprepisovala?")
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
  // (AI-assisted, prompt: "jak v ifjcode26 napsat cyklus while s vyhodnocenim
  // podminky na zasobniku pres JUMPIFEQS a unikatnimi navestimi pro zacatek a
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
  // container nodes (NODE_PROGRAM, NODE_BLOCK)
  else if (node->node_type == NODE_PROGRAM || node->node_type == NODE_BLOCK) {
    // just loop thru all children in order from left to right
    for (int i = 0; i < node->child_count; i++) {
      generate_statement(node->child_arr[i]);
    }
  }
  // standalone expression statement (e.g. (), 1 / 0, a + b)
  else if (is_value_node(node)) {
    generate_expression(node);
    printf("POPS GF@temp_result\n");
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
  // extra helper vars needed for subStr loop and bounds checks
  printf("DEFVAR GF@temp_out\n");
  printf("DEFVAR GF@temp_ch\n");
  printf("DEFVAR GF@temp_len\n");
  printf("DEFVAR GF@temp_cmp\n");

  // jump to $$entry_main wrapper which calls @main and exits cleanly
  printf("JUMP $$entry_main\n");

  // start traversing the AST from root
  if (root != NULL) {
    generate_statement(root);
  }
}