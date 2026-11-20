// Project: prekladac-ifj26
// File: semantics.c
// Author: Martin Vaško (vaskom00)

// Implementation of abstract syntax tree = AST and semantic analysis functions

#include "semantics.h"
#include "bst.h" // For recursion check
#include "error_codes.h"
#include "semantics_priv.h"
#include "symtable.h"

#include <assert.h>
#include <errno.h> // For conversion errors
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macro for checking for errors and returning if error occurred
#define RETURN_ON_ERROR(error_code, error_msg)                                 \
  do {                                                                         \
    ErrCode evaluated_err = (error_code);                                      \
    if (evaluated_err != SUCCESS) {                                            \
      fprintf(stderr, "Error: %s\n", error_msg);                               \
      return evaluated_err;                                                    \
    }                                                                          \
  } while (0)

// Variable that holds currently resolved function definition.
// It's only used to simplify comparing data types of functions
// and their return statements.
static ASTNode current_func_def;

// Variable that holds all function definition nodes. Used for looking up func
// def nodes using their identifier strings.
static BSTNode func_def_list;

// Function definitions

ErrCode AST_init_node(ASTNode *node, NodeType node_type, DataType data_type,
                      char *node_data) {

  // Variable that temporarily holds error codes
  ErrCode err = SUCCESS;

  // Allocate memory for new node
  ASTNode new_node = malloc(sizeof(struct ast_node_struct));
  // Check success
  if (new_node == NULL)
    return ERR_INTERNAL;

  // Initialize node
  // These types MUST be initialized first!
  new_node->node_type = node_type;
  new_node->data_type = data_type;

  // Figure out type of data node_data is, and assign it
  if (node_type == NODE_IDENTIFIER) {
    // Data is identifier
    err = set_string_node_data(new_node, node_data);
  }

  else if (node_type == NODE_LITERAL) {
    // Check data types: Integer
    if (data_type == DT_INTEGER) {
      err = set_integer_node_data(new_node, node_data);
    }

    // Double
    else if (data_type == DT_DOUBLE) {
      err = set_double_node_data(new_node, node_data);
    }

    // String
    else if (data_type == DT_STRING) {
      err = set_string_node_data(new_node, node_data);
    }
  } else {
    // Don't set anything
  }

  // Check for errors
  if (err != SUCCESS) {
    free(new_node);
    return err;
  }

  // These values might be modified later
  new_node->symbol_ptr = NULL;
  new_node->child_count = 0;
  new_node->child_limit = 0;
  new_node->child_arr = NULL;
  new_node->was_resolved = false;

  // Don't forget to set the node!
  *node = new_node;

  return SUCCESS;
}

static ErrCode set_integer_node_data(ASTNode node, char *data_string) {

  assert(node != NULL);
  assert(data_string != NULL);
  // Data string must not be empty
  assert(strlen(data_string) > 0);

  // Pointers that help check correct conversion
  char *string_end = &data_string[strlen(data_string)];
  char *conv_end = NULL;

  errno = 0; // Reset errno

  // Conversion to long long
  long long int_data = strtoll(data_string, &conv_end, 10);

  // Check errors
  // Value out of range, and narrowing long long to 32 bit int
  if (errno == ERANGE || int_data > INT32_MAX || int_data < INT32_MIN) {
    errno = 0; // Reset errno
    return ERR_INCOMPATIBLE_TYPE;
  }
  // String wasn't fully converted
  if (string_end != conv_end) {
    return ERR_INCOMPATIBLE_TYPE;
  }

  // Assign data
  node->data.int_val = int_data;

  return SUCCESS;
}

static ErrCode set_double_node_data(ASTNode node, char *data_string) {

  assert(node != NULL);
  assert(data_string != NULL);
  // Data string must not be empty
  assert(strlen(data_string) > 0);

  // Pointers that help check correct conversion
  char *string_end = &data_string[strlen(data_string)];
  char *conv_end = NULL;

  errno = 0; // Reset errno

  // Conversion
  double dbl_data = strtod(data_string, &conv_end);

  // Check errors
  // Value out of range
  if (errno == ERANGE) {
    errno = 0; // Reset errno
    return ERR_INCOMPATIBLE_TYPE;
  }
  // String wasn't fully converted
  if (string_end != conv_end) {
    return ERR_INCOMPATIBLE_TYPE;
  }

  // Assign data
  node->data.double_val = dbl_data;

  return SUCCESS;
}

static ErrCode set_string_node_data(ASTNode node, char *data_string) {

  assert(node != NULL);
  assert(data_string != NULL);
  // Identifier cannot be an empty string
  if (node->node_type == NODE_IDENTIFIER) {
    assert(strlen(data_string) > 0);
  }

  // node_data should be symbol identifier
  node->data.string_val = malloc(sizeof(char) * (strlen(data_string) + 1));
  // Check success
  if (node->data.string_val == NULL) {
    return ERR_INTERNAL; // Memory error
  }
  // Copy string
  strcpy(node->data.string_val, data_string);

  return SUCCESS;
}

ErrCode AST_node_add_child(ASTNode parent, ASTNode child) {

  // Assertions
  assert(parent != NULL);
  assert(child != NULL);

  // Index where child will be inserted into array
  int i = parent->child_count;

  int new_child_count = parent->child_count + 1;

  // Dynamic array allocation
  if (parent->child_limit == 0) {
    // First child node
    parent->child_limit = 2; // 2 is a common amount of AST child nodes
    parent->child_arr = malloc(sizeof(ASTNode) * parent->child_limit);
    if (parent->child_arr == NULL) {
      return ERR_INTERNAL;
    }

  } else if (parent->child_limit < new_child_count) {
    // Out of limit
    parent->child_limit *= 2; // Double the limit
    // Need to realloc
    ASTNode *new_arr =
        realloc(parent->child_arr, sizeof(ASTNode) * parent->child_limit);
    if (new_arr == NULL) {
      return ERR_INTERNAL;
    }

    // Successful realloc
    parent->child_arr = new_arr;
  }

  // Update parent count
  parent->child_count += 1;

  // After making room, assign child node
  parent->child_arr[i] = child;

  // Success
  return SUCCESS;
}

ErrCode semantic_analysis(ASTNode root, SymbTables *st) {

  // Error code return variable
  ErrCode err = SUCCESS;

  // Initialize global variable
  current_func_def = NULL;

  // Check unallowed return statements and recursion, right now we can tell
  // if function type was explicitly declared
  err = check_other_semantics(root);
  RETURN_ON_ERROR(err, "found invalid return statement or invalid recursion.");

  func_def_list = BST_new();
  // Fill in symbol table with function declarations
  // Also fills global func_def_list!
  err = define_func_symbols(root, st);
  if (err != SUCCESS) {
    BST_destroy_tree(func_def_list);
    RETURN_ON_ERROR(err, "failed defining function symbols.");
  }
  // Fill in symbol table with local symbols (variables),
  // do all type checking, type inference and semantic checks.
  // Recursively calls "resolve" functions on all valid child nodes.
  err = resolve_AST(root, st);
  if (err != SUCCESS) {
    BST_destroy_tree(func_def_list);
    RETURN_ON_ERROR(err, "program contains semantic error.");
  }

  // Types are checked, symbols are defined, we are DONE
  BST_destroy_tree(func_def_list);
  return SUCCESS;
}

ErrCode check_other_semantics(ASTNode root) {

  assert(root != NULL);

  // Direct children of root are only function definitions
  for (int i = 0; i < root->child_count; i++) {

    // Get function def node
    ASTNode func_def = root->child_arr[i];

    // Assert correct AST structure
    assert(func_def->node_type == NODE_FUNC_DEF ||
           func_def->node_type == NODE_MAIN_FUNC_DEF);
    assert(func_def->child_count == 3);

    // Data type isn't set explicitly -> return or recursion is forbidden
    if (func_def->data_type == DT_UNDEF) {

      ASTNode func_body = func_def->child_arr[I_FBODY];

      // Function that is just an expression cannot fail this
      if (func_body->node_type == NODE_EXPR) {
        continue; // Check next
      }

      // Only a block can hold return statement
      if (func_body->node_type == NODE_BLOCK) {
        if (contains_return(func_body)) {

          char *bad_fun = func_def->child_arr[I_FD_ID]->data.string_val;
          fprintf(stderr, "%s: Contains return.\n", bad_fun);
          return ERR_OTHER_SEMANTIC;
        }
      }

      // Check if can contain recursive func calls
      if (func_body->node_type == NODE_BLOCK ||
          func_body->node_type == NODE_FUNC_CALL) {

        char *func_name = func_def->child_arr[I_FD_ID]->data.string_val;

        // Run detection
        ErrCode err = check_recursion(func_body, func_name, root);
        if (err != SUCCESS)
          return err;

      } // if can contain recursion
    }
    // else
    // - data type is set explicitly and can contain return/recursion

  } // child check for loop

  // Checks succeeded
  return SUCCESS;
}

static bool contains_return(ASTNode block) {

  // Only check blocks
  assert(block->node_type == NODE_BLOCK);

  // Check every child
  for (int i = 0; i < block->child_count; i++) {

    ASTNode child = block->child_arr[i];

    // Return statement
    if (child->node_type == NODE_RETURN) {
      return true;

      // If
    } else if (child->node_type == NODE_IF) {
      // Search blocks inside if
      if (contains_return(child->child_arr[I_IB1]) ||
          contains_return(child->child_arr[I_IB2]))
        return true;

      // While
    } else if (child->node_type == NODE_WHILE) {
      // Search block inside loop
      if (contains_return(child->child_arr[I_WB]))
        return true;
    }
  }

  return false;
}

static ErrCode check_recursion(ASTNode func_body, char *func_name,
                               ASTNode root) {

  // Check recursion (direct or indirect)

  // Recursively find all function calls and save them to LIST
  // STEP 2:
  // Compare them to `func_name`
  // If equal, return true
  // If not equal, recursively find all function calls from functions in the
  // LIST. If any new funcalls were inserted into LIST, repeat from STEP 2

  // List of function calls which are "accessible"
  BSTNode call_list = BST_new();

  // Keep track of amount of calls
  int prev_total = 0;
  int total = 0;

  int subtotal = find_func_calls(func_body, &call_list);
  // Check errors
  if (subtotal < 0) {
    BST_destroy_tree(call_list);
    return ERR_INTERNAL;
  }

  // Add to total
  total += subtotal;

  // No function calls found
  if (total == 0) {
    BST_destroy_tree(call_list);
    return SUCCESS;
  }

  // Check if we found recursion
  if (BST_get_data(call_list, func_name) != NULL) {
    // Invalid recursion found!
    BST_destroy_tree(call_list);
    fprintf(stderr, "Error: Contains direct recursion.\n");
    return ERR_OTHER_SEMANTIC;
    // More checking :(
  } else {
    do {

      // Update condition value
      prev_total = total;

      // We need to find possible indirect recursion!
      // Check every function DEFINITION
      for (int i = 0; i < root->child_count; i++) {

        ASTNode called_func_def = root->child_arr[i];

        char *called_name =
            called_func_def->child_arr[I_FD_ID]->data.string_val;

        // If called_func was called
        if (BST_get_data(call_list, called_name) != NULL) {

          ASTNode called_func_body = called_func_def->child_arr[I_FBODY];
          int subtotal = find_func_calls(called_func_body, &call_list);

          // COPIED CODE EWWWW but its entirely not identical
          // Check errors
          if (subtotal < 0) {
            BST_destroy_tree(call_list);
            return ERR_INTERNAL;
          }

          // Add to total
          total += subtotal;

          // Check if we found recursion
          if (BST_get_data(call_list, func_name) != NULL) {

            // Invalid recursion found!
            BST_destroy_tree(call_list);
            fprintf(stderr, "Error: Contains indirect recursion.\n");
            return ERR_OTHER_SEMANTIC;
          }

        } // if called func

      } // for func_defs

      // Repeat while we keep finding new unique func calls
    } while (prev_total < total);
  } // recursion check

  // Don't forget to destroy tree
  BST_destroy_tree(call_list);
  return SUCCESS;
}

int find_func_calls(ASTNode func_body, BSTNode *call_list) {

  // Assertions
  assert(func_body != NULL);
  assert(func_body->node_type == NODE_BLOCK ||
         func_body->node_type == NODE_FUNC_CALL ||
         func_body->node_type == NODE_EXPR);
  assert(call_list != NULL);

  // Expression
  if (func_body->node_type == NODE_EXPR) {
    // No function calls in expression
    return 0;
  }

  // Function call
  else if (func_body->node_type == NODE_FUNC_CALL) {

    char *func_name = func_body->child_arr[I_FC_ID]->data.string_val;

    // Check if it already exists
    if (BST_get_data(*call_list, func_name) != NULL) {
      return 0; // Function call with the same name already exists
    }

    bool placeholder = true; // Just something to insert into BST
    bool success = BST_insert(call_list, func_name, &placeholder, sizeof(bool));
    if (success == false)
      return -1; // Code for error
    else
      return 1; // New function call found
  }

  // Block
  else if (func_body->node_type == NODE_BLOCK) {

    int total = 0;

    // Check every child
    for (int i = 0; i < func_body->child_count; i++) {

      ASTNode child = func_body->child_arr[i];
      int subtotal = 0;

      // Recursively find calls based on node type of statement nodes
      switch (child->node_type) {

      case NODE_EXPR:
      case NODE_FUNC_CALL: {
        subtotal = find_func_calls(child, call_list);
        break;
      }

      case NODE_VAR_DEF:
      case NODE_VAL_DEF:
      case NODE_ASSIGNMENT: {
        subtotal = find_func_calls(child->child_arr[I_RHS], call_list);
        break;
      }

      case NODE_RETURN: {
        // Return MUST have child expr or func call
        subtotal = find_func_calls(child->child_arr[I_RET], call_list);
        break;
      }

      case NODE_WHILE: {
        subtotal = find_func_calls(child->child_arr[I_WB], call_list);
        break;
      }

      case NODE_IF: {
        int sub1 = find_func_calls(child->child_arr[I_IB1], call_list);
        int sub2 = find_func_calls(child->child_arr[I_IB2], call_list);
        // Check errors from both blocks
        if (sub1 < 0 || sub2 < 0) {
          subtotal = -1;
        } else {
          subtotal = sub1 + sub2;
        }
        break;
      }

      default:
        fprintf(stderr, "Programmer error: recursion: no matching type!\n");
        return -1;
        break;
      }

      if (subtotal < 0)
        return -1; // Error occurred
      else
        total += subtotal; // Add to total
    } // end child check

    return total;
  }
  // else
  // There can't be any other valid node type.
  return -1;
}

ErrCode define_func_symbols(ASTNode root, SymbTables *st) {

  assert(root != NULL);
  // symbol tables aren't empty
  // it should be in global scope
  assert(st->stack_top != NULL);
  assert(st->stack_top->prev_scope == NULL);
  ErrCode err = SUCCESS;

  bool main_was_defined = false;

  // Define every function
  for (int i = 0; i < root->child_count; i++) {

    // Func definition
    ASTNode func_def = root->child_arr[i];
    // Func identifier
    ASTNode func_idtf = func_def->child_arr[I_FD_ID];
    // Func param list
    ASTNode func_par = func_def->child_arr[I_FD_PAR];

    // Sanity checks
    assert(func_def->node_type == NODE_FUNC_DEF ||
           func_def->node_type == NODE_MAIN_FUNC_DEF);
    // A convention I decided to follow, even though it's useless
    assert(func_def->data_type == func_idtf->data_type);

    // Insert pointer to func_def node into global list
    err = BST_insert(&func_def_list, func_idtf->data.string_val, func_def,
                     sizeof(ASTNode));
    RETURN_ON_ERROR(err, ERR_INTERNAL);

    SymbType func_sym_type = SB_FUNC;

    // Set proper symbol type
    if (func_def->node_type == NODE_MAIN_FUNC_DEF) {

      func_sym_type = SB_MAIN_FUNC;

      // Main function checks

      // Multiple main functions
      if (main_was_defined == true) {
        RETURN_ON_ERROR(ERR_REDEFINITION, "Multiple main functions.");
      }
      main_was_defined = true;

      // Main func must have zero parameters
      if (func_par->child_count != 0) {
        RETURN_ON_ERROR(ERR_FUN_ARGS, "Main function has parameters.");
      }

      // Main func must have return data type Unit
      // Parser will create AST with all information it has
      // If data type wasn't defined we'll assume it's Unit
      // Then later, type checks will fail if result is different.
      if (func_def->data_type == DT_UNDEF) {
        func_def->data_type = DT_UNIT;
        func_idtf->data_type = DT_UNIT;

        // else if it has wrong explicitly defined data type
      } else if (func_def->data_type != DT_UNIT) {
        RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE,
                        "Main function has wrong return data type.");
      }

      // At end of this function we check if main_was_defined.

      // Else function isn't main
    } else if (func_def->node_type == NODE_FUNC_DEF) {
      func_sym_type = SB_FUNC;
    }

    int par_count = func_par->child_count;
    DataType *par_arr = NULL;

    // Create parameter array if needed
    if (par_count > 0) {
      // Allocate memory
      par_arr = malloc(sizeof(DataType) * par_count);
      if (par_arr == NULL) {
        RETURN_ON_ERROR(ERR_INTERNAL, "Parameter array filling.");
      }

      // Populate array
      for (int p = 0; p < par_count; p++) {
        par_arr[p] = func_par->child_arr[p]->data_type;
      }
    }

    // Fill symbol data
    SymbDraft func_draft = {.identifier = func_idtf->data.string_val,
                            .symb_type = func_sym_type,
                            .data_type = func_idtf->data_type,
                            .param_count = par_count,
                            .param_type_arr = par_arr};

    // Try defining it - this also checks redefinition!
    err = ST_define_symbol(st, &func_draft);
    free(par_arr); // every malloc has a free :)
    RETURN_ON_ERROR(err, "occurred when defining function symbol.");

    // After defining symbol, we need to save pointer to it
    Symbol *func_symb = ST_get_symbol(st, func_idtf->data.string_val);

    // Function symbol should never be NULL
    assert(func_symb != NULL);

    // Save pointer to symbol
    func_idtf->symbol_ptr = func_symb;
  }

  // There must be one main function
  if (main_was_defined == false) {
    RETURN_ON_ERROR(ERR_UNDEF_SYM, "Missing main function.");
  }

  // All good
  return SUCCESS;
}

static ErrCode infer_or_check_types(ASTNode lhs, ASTNode rhs, SymbTables *st) {

  // Assert nodes and define err
  assert(lhs != NULL);
  // Left side must be identifier
  assert(lhs->node_type == NODE_IDENTIFIER);

  // Right side must have known type
  assert(rhs != NULL);
  assert(rhs->data_type != DT_UNDEF);

  ErrCode err = SUCCESS;

  /*
    4 situacie
    poznam typ funkcie aj typ tela
    poznam typ funkcie ale nepoznam typ tela
    nepoznam typ funkcie ale poznam typ tela
    nepoznam typ funkcie ani tela

    1 - porovnam
    2 - nestane sa nikdy, telo sa odvodi funkciami
    3 - idk
    4 - iste ako 2

    cize
    1 poznam typ funkcie aj typ tela
    2 nepoznam typ funkcie ale poznam typ tela

    1 - kontrola ci su rovnake
    2 - nastavim typ funkcie

    */

  return SUCCESS;
}

static ErrCode resolve_any_func_body(ASTNode func_body, SymbTables *st) {

  // Assert node and define err
  assert(func_body != NULL);
  ErrCode err = SUCCESS;

  // Find appropriate resolve function
  switch (func_body->node_type) {

  case NODE_BLOCK: {
    err = resolve_block(func_body, st, true);
    break;
  }
  case NODE_FUNC_CALL: {
    err = resolve_func_call(func_body, st);
    break;
  }
  case NODE_EXPR: {
    err = resolve_expression(func_body, st);
    break;
  }

  // If no function was matched, it's my fault
  default: {
    fprintf(stderr, "Failed matching resolve func for func body.\n");
    err = ERR_INTERNAL;
    break;
  }
  }

  return err;
}

static ErrCode resolve_any_stmt(ASTNode stmt, SymbTables *st) {

  // Assert node and define err
  assert(stmt != NULL);
  ErrCode err = SUCCESS;

  // Find appropriate resolve function
  switch (stmt->node_type) {

  case NODE_FUNC_CALL: {
    err = resolve_func_call(stmt, st);
    break;
  }
  case NODE_EXPR: {
    err = resolve_expression(stmt, st);
    break;
  }
  case NODE_VAR_DEF: {
    err = resolve_var_def(stmt, st);
    break;
  }
  case NODE_VAL_DEF: {
    err = resolve_val_def(stmt, st);
    break;
  }
  case NODE_ASSIGNMENT: {
    err = resolve_assignment(stmt, st);
    break;
  }
  case NODE_IF: {
    err = resolve_if(stmt, st, false);
    break;
  }
  case NODE_WHILE: {
    err = resolve_while(stmt, st);
    break;
  }
  case NODE_RETURN: {
    err = resolve_return(stmt, st);
    break;
  }
  // If no function was matched, it's my fault
  default: {
    fprintf(stderr, "Failed matching resolve func for statement.\n");
    err = ERR_INTERNAL;
    break;
  }
  }

  return err;
}

static ErrCode resolve_any_rhs(ASTNode rhs_node, SymbTables *st) {

  // Assert node and define err
  assert(rhs_node != NULL);
  ErrCode err = SUCCESS;

  // Find appropriate resolve function
  switch (rhs_node->node_type) {

  case NODE_FUNC_CALL: {
    err = resolve_func_call(rhs_node, st);
    break;
  }
  case NODE_EXPR: {
    err = resolve_expression(rhs_node, st);
    break;
  }

  // If no function was matched, it's my fault
  default: {
    fprintf(stderr, "Failed matching resolve func for rhs node.\n");
    err = ERR_INTERNAL;
    break;
  }
  }

  return err;
}

static ErrCode resolve_any_term(ASTNode term, SymbTables *st) {

  // Assert node and define err
  assert(term != NULL);
  ErrCode err = SUCCESS;

  // Find appropriate resolve function
  switch (term->node_type) {

  case NODE_UN_MINUS:
  case NODE_UN_PLUS: {
    err = resolve_unary_op(term, st);
    break;
  }
  case NODE_IDENTIFIER: {
    err = resolve_rhs_var_id(term, st);
    break;
  }
  case NODE_LITERAL: {
    // We know everything there is about literals
    // Nothing to do
    break;
  }

  // If no function was matched, it's my fault
  default: {
    fprintf(stderr, "Failed matching resolve func for term.\n");
    err = ERR_INTERNAL;
    break;
  }
  }

  return err;
}
/*
static ErrCode resolve_any_TODO(ASTNode node, SymbTables *st) {

  // Assert node and define err
  assert(node != NULL);
  ErrCode err = SUCCESS;

  // Find appropriate resolve function
  switch (node->node_type) {

  case NODE_ADD:
  case NODE_SUB:
  case NODE_MUL:
  case NODE_DIV: {
    err = resolve_binary_op(node, st);
    break;
  }
  case NODE_UN_PLUS:
  case NODE_UN_MINUS: {
    err = resolve_unary_op(node, st);
    break;
  }


  // If no function was matched, it's my fault
  default: {
    fprintf(stderr, "PROGRAMMING ERR: couldn't match node with resolve\n");
    err = ERR_INTERNAL;
    break;
  }
  }

  // Returns SUCCESS or error code
  return err;
}

*/

static ErrCode resolve_AST(ASTNode root, SymbTables *st) {

  // symbol tables aren't empty
  // it should be in global scope
  assert(st->stack_top != NULL);
  assert(st->stack_top->prev_scope == NULL);

  // Assert node and define err
  assert(root != NULL);
  assert(root->node_type == NODE_PROGRAM);
  ErrCode err = SUCCESS;

  // Loop over every function
  for (int i = 0; i < root->child_count; i++) {

    ASTNode func_def = root->child_arr[i];

    // Avoid repeated resolves
    if (func_def->was_resolved == false) {
      err = resolve_func_def(root->child_arr[i], st);
      RETURN_ON_ERROR(err, "inside AST");
    }

  } // func_def loop

  // AST is completed.
  return SUCCESS;
}

static ErrCode resolve_func_def(ASTNode func_def, SymbTables *st) {

  // Assert node and define err
  assert(func_def != NULL);
  assert(func_def->node_type == NODE_MAIN_FUNC_DEF ||
         func_def->node_type == NODE_FUNC_DEF);
  ErrCode err = SUCCESS;

  // Func definition child nodes
  ASTNode func_id = func_def->child_arr[I_FD_ID];
  ASTNode func_body = func_def->child_arr[I_FBODY];
  ASTNode func_par = func_def->child_arr[I_FD_PAR];
  int par_count = func_par->child_count;

  // Func def identifiers must already be defined
  assert(func_id->symbol_ptr != NULL);

  // Set global variable to help resolve return statements
  ASTNode prev_func_def = current_func_def; // Save previous value
  current_func_def = func_def;              // Set new

  // Helper variable for scope enter/exit
  bool scope_entered = false;

  // It's worth entering scope only when func has parameters
  // or its body is a block!
  if (par_count > 0 || func_body->node_type == NODE_BLOCK) {

    // Enter local scope
    err = ST_enter_scope(st);
    RETURN_ON_ERROR(err, "occurred on enter_scope in func_def");

    scope_entered = true; // So we know that we should exit scope too
  }

  // Define parameters symbols
  for (int p = 0; p < par_count; p++) {

    // Parameter
    ASTNode par_id = func_par->child_arr[p];

    // Verify correctness of func symbols
    assert(par_id->data_type == func_id->symbol_ptr->param_type_arr[p]);

    // symbol type is SB_VAL because params are unmodifiable
    err = define_variable_symbol(par_id, SB_VAL, st);
    RETURN_ON_ERROR(err, "occurred on defining symbol in func_def.");
  }

  // Resolve and check function body
  err = resolve_any_func_body(func_body, st);
  RETURN_ON_ERROR(err, "inside func_def");

  // After entering local scope, don't forget to exit it!
  if (scope_entered == true) {
    ST_exit_scope(st);
  }

  // If data type is set explicitly, and function body is a block ending with
  // if statement, block will have inferred type Unit. This can cause an issue
  // if the if-statement ends with return in every branch. We don't infer type
  // of these returns into the if-statement, we always set if-statement to have
  // data type Unit. Then, there could be incompatible types between explicitly
  // set function type that is NOT Unit and type of block which IS Unit, even
  // though according to specification, it should be vaild.
  int last = func_body->child_count - 1;
  assert(last >= 0);
  if (func_id->node_type != DT_UNDEF && func_id->node_type != DT_UNIT &&
      func_body->node_type == NODE_BLOCK &&
      func_body->child_arr[last]->node_type == NODE_IF) {

    // DON'T CHECK TYPES, they are checked in return statements.

  } else {
    // Check types of both sides, or infer type
    err = infer_or_check_types(func_id, func_body, st);
    RETURN_ON_ERROR(err, "type error in func_def");
  }

  // Function def nodes data types is the same as its identifier node
  // It's the only node that shares type with identifier node
  func_def->data_type = func_id->data_type;

  // Before returning, revert global flag to its previous state
  current_func_def = prev_func_def;

  // Set flag so we don't resolve this again
  func_def->was_resolved = true;

  return SUCCESS;
}

static ErrCode resolve_block(ASTNode block, SymbTables *st, bool check_end_if) {

  // Assert node and define err
  assert(block != NULL);
  assert(block->node_type == NODE_BLOCK);
  ErrCode err = SUCCESS;

  // Loop over every statement
  for (int i = 0; i < block->child_count; i++) {

    ASTNode stmt = block->child_arr[i];

    // Treat special case where last statement of function body is an if stmt
    if (check_end_if == true && i == block->child_count - 1 &&
        stmt->node_type == NODE_IF) {

      err = resolve_if(stmt, st, true);
      RETURN_ON_ERROR(err, "inside block, ending if stmt");
    }
    // Otherwise normally resolve statement
    else {
      // Resolve each one
      err = resolve_any_stmt(stmt, st);
      RETURN_ON_ERROR(err, "inside block");
    }

    // Last statement of block is result of block!
    if (i == block->child_count - 1) {
      block->data_type = stmt->data_type;
    }
  }

  return SUCCESS;
}

static ErrCode resolve_func_call(ASTNode func_call, SymbTables *st) {

  // Assert node and define err
  assert(func_call != NULL);
  assert(func_call->node_type == NODE_FUNC_CALL);
  ErrCode err = SUCCESS;

  // Func call child nodes
  ASTNode func_id = func_call->child_arr[I_FC_ID];
  ASTNode func_arg = func_call->child_arr[I_FC_ARG];
  int arg_count = func_arg->child_count;

  // Function symbols are made ahead of time
  // Check if symbol exists
  Symbol *func_symb = ST_get_symbol(st, func_id->data.string_val);
  if (func_symb == NULL) {
    RETURN_ON_ERROR(ERR_UNDEF_SYM, "called an undefined function.");
  }

  // If function call has undefined data type, we simply resolve it first!
  if (func_symb->data_type == DT_UNDEF) {

    // Save scope
    LNode local_stack = ST_save_scope_stack(st);

    // Get func def node to resolve
    ASTNode unresolved = BST_get_data(func_def_list, func_id->data.string_val);
    assert(unresolved != NULL); // This cannot fail!

    err = resolve_func_def(unresolved, st);
    RETURN_ON_ERROR(err, "inside func call");

    // Restore scope and continue like nothing happened!
    ST_restore_scope_stack(st, local_stack);
  }
  // Now funtion call must have defined data type
  assert(func_symb->data_type != DT_UNDEF);

  // Set data type
  func_call->data_type = func_symb->data_type;
  func_id->data_type = func_symb->data_type;

  // Check correct argument count
  if (arg_count != func_symb->param_count) {
    RETURN_ON_ERROR(ERR_FUN_ARGS,
                    "function called with wrong amount of arguments");
  }

  // Resolve and verify argument types
  for (int a = 0; a < arg_count; a++) {

    // Argument
    ASTNode arg = func_arg->child_arr[a];

    // Resolve its type
    err = resolve_any_term(arg, st);
    RETURN_ON_ERROR(err, "inside function call in argument");

    assert(arg->data_type != DT_UNDEF);

    // Check if it matches param type
    DataType arg_type = arg->data_type;
    DataType param_type = func_symb->param_type_arr[a];
    if (arg_type != param_type) {

      // Check implicit conversion
      if (arg_type == DT_INTEGER && param_type == DT_DOUBLE) {

        // TODO IMPLICIT CONVERSION
      } else {
        RETURN_ON_ERROR(ERR_FUN_ARGS,
                        "function called with wrong data type of argument");
      }
    }
    // else data types are equal
  }

  return SUCCESS;
}

static ErrCode resolve_expression(ASTNode expr, SymbTables *st) {

  // Assert node and define err
  assert(expr != NULL);
  assert(expr->node_type == NODE_EXPR);
  ErrCode err = SUCCESS;

  return SUCCESS;
}

// This is intended only for variable identifiers in expressions and arguments
static ErrCode resolve_rhs_var_id(ASTNode idtf, SymbTables *st) {

  // Assert node and define err
  assert(idtf != NULL);
  assert(idtf->node_type == NODE_IDENTIFIER);
  ErrCode err = SUCCESS;

  Symbol *var_symb = ST_get_symbol(st, idtf->data.string_val);
  // Variable must be defined by now
  if (var_symb == NULL) {
    RETURN_ON_ERROR(ERR_UNDEF_SYM, "undefined variable.");
  }

  // Data type should be defined with the symbol
  assert(var_symb->data_type != DT_UNDEF);

  // Infer data type of symbol
  idtf->data_type = var_symb->data_type;

  return SUCCESS;
}

static ErrCode resolve_var_def(ASTNode var_def, SymbTables *st) {

  // Assert node and define err
  assert(var_def != NULL);
  assert(var_def->node_type == NODE_VAR_DEF);
  assert(var_def->data_type == DT_UNIT);
  ErrCode err = SUCCESS;

  // Define symbol
  ASTNode var_def_id = var_def->child_arr[I_LHS];
  err = define_variable_symbol(var_def_id, SB_VAR, st);
  RETURN_ON_ERROR(err, "inside var_def");

  // Resolve right side of definition
  ASTNode rhs_node = var_def->child_arr[I_RHS];
  err = resolve_any_rhs(rhs_node, st);
  RETURN_ON_ERROR(err, "inside var_def");

  // Do type check
  err = infer_or_check_types(var_def_id, rhs_node, st);
  RETURN_ON_ERROR(err, "inside var_def");

  return SUCCESS;
}

static ErrCode resolve_val_def(ASTNode val_def, SymbTables *st) {

  // Assert node and define err
  assert(val_def != NULL);
  assert(val_def->node_type == NODE_VAL_DEF);
  assert(val_def->data_type == DT_UNIT);
  ErrCode err = SUCCESS;

  // Define symbol
  ASTNode val_def_id = val_def->child_arr[I_LHS];
  err = define_variable_symbol(val_def_id, SB_VAL, st);
  RETURN_ON_ERROR(err, "inside val_def");

  // Resolve right side of definition
  ASTNode rhs_node = val_def->child_arr[I_RHS];
  err = resolve_any_rhs(rhs_node, st);
  RETURN_ON_ERROR(err, "inside val_def");

  // Do type check
  err = infer_or_check_types(val_def_id, rhs_node, st);
  RETURN_ON_ERROR(err, "inside val_def");

  return SUCCESS;
}

static ErrCode resolve_assignment(ASTNode assign, SymbTables *st) {

  // Assert node and define err
  assert(assign != NULL);
  assert(assign->node_type == NODE_ASSIGNMENT);
  assert(assign->data_type == DT_UNIT);
  ErrCode err = SUCCESS;

  return SUCCESS;
}

static ErrCode resolve_binary_op(ASTNode bin_op, SymbTables *st) {

  // Assert node and define err
  assert(bin_op != NULL);
  assert(bin_op->node_type == NODE_ADD || bin_op->node_type == NODE_SUB ||
         bin_op->node_type == NODE_MUL || bin_op->node_type == NODE_DIV);
  ErrCode err = SUCCESS;

  return SUCCESS;
}

static ErrCode resolve_unary_op(ASTNode un_op, SymbTables *st) {

  // Assert node and define err
  assert(un_op != NULL);
  assert(un_op->node_type == NODE_UN_PLUS || un_op->node_type == NODE_UN_MINUS);
  ErrCode err = SUCCESS;

  return SUCCESS;
}

static ErrCode resolve_condition(ASTNode cond, SymbTables *st) {

  // Assert node and define err
  assert(cond != NULL);
  assert(cond->node_type == NODE_COND);
  assert(cond->data_type == DT_BOOL);
  assert(cond->child_count == 1);
  ErrCode err = SUCCESS;

  // Child guaranteed to be relation operator
  ASTNode rel_op = cond->child_arr[I_OP];

  // Assert op
  assert(rel_op != NULL);
  assert(rel_op->node_type == NODE_EQ || rel_op->node_type == NODE_NOT_EQ ||
         rel_op->node_type == NODE_GT || rel_op->node_type == NODE_GT_EQ ||
         rel_op->node_type == NODE_LT || rel_op->node_type == NODE_LT_EQ);

  
  ASTNode left_expr = rel_op->child_arr[I_L_O];
  ASTNode right_expr = rel_op->child_arr[I_R_O];

  // Resolve both sides
  err = resolve_expression(left_expr, st);
  RETURN_ON_ERROR(err, "inside condition, left expression");
  err = resolve_expression(right_expr, st);
  RETURN_ON_ERROR(err, "inside condition, right expression");

  // Data types
  DataType left_dt = left_expr->data_type;
  DataType right_dt = left_expr->data_type;
  assert(left_dt != DT_UNDEF);
  assert(right_dt != DT_UNDEF);
  // Check types
  if (left_dt != right_dt) {

    // Check for possible implicit conversion
    if (left_dt == DT_DOUBLE && right_dt == DT_INTEGER) {

      // TODO Convert right_expr to double
    }
    else if (left_dt == DT_INTEGER && right_dt == DT_DOUBLE) {

      // TODO Convert left_expr to double
    }
    else {
      // Type incompatibility
      RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE, "incompatible types in condition.");
    }
  }
  // Else types are compatible

  return SUCCESS;
}

static ErrCode resolve_if(ASTNode if_stmt, SymbTables *st,
                          bool is_func_result) {

  // Assert node and define err
  assert(if_stmt != NULL);
  assert(if_stmt->node_type == NODE_IF);
  assert(if_stmt->data_type == DT_UNIT);
  ErrCode err = SUCCESS;

  // Set child nodes
  ASTNode cond = if_stmt->child_arr[I_COND];
  ASTNode then_block = if_stmt->child_arr[I_IB1];
  ASTNode else_block = if_stmt->child_arr[I_IB2];

  // Condition
  err = resolve_condition(cond, st);
  RETURN_ON_ERROR(err, "inside if condition");

  // SCOPE START
  err = ST_enter_scope(st);
  RETURN_ON_ERROR(err, "inside if, on entering scope of then");

  // Block then
  err = resolve_block(then_block, st, is_func_result);
  RETURN_ON_ERROR(err, "inside if then block");

  ST_exit_scope(st);
  // SCOPE END

  // SCOPE START
  err = ST_enter_scope(st);
  RETURN_ON_ERROR(err, "inside if, on entering scope of else");

  // Block else
  err = resolve_block(else_block, st, is_func_result);
  RETURN_ON_ERROR(err, "inside if else block");

  ST_exit_scope(st);
  // SCOPE END

  // Treat edge case where if statement is result of func body
  if (is_func_result == true) {

    // One of the following 3 conditions must be true:
    // 1 - return type is explicitly Unit
    bool func_is_unit = (current_func_def->data_type == DT_UNIT);

    // 2 - all branches end with return
    // I understood it as: both blocks are guaranteed to execute return
    bool ends_with_return = (block_ends_with_return(then_block) &&
                             block_ends_with_return(else_block));

    // 3 - return type isn't explicitly set and results of all branches
    // have result type Unit
    bool third_cond =
        (current_func_def->data_type == DT_UNDEF &&
         then_block->data_type == DT_UNIT && else_block->data_type == DT_UNIT);

    // Check validity
    bool valid_if_result = (func_is_unit || ends_with_return || third_cond);
    if (valid_if_result == false) {
      RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE,
                      "function body result if statement is invalid.");
    }
  }

  return SUCCESS;
}

static bool block_ends_with_return(ASTNode block) {

  // Assert node and define err
  assert(block != NULL);
  assert(block->node_type == NODE_BLOCK);

  // Loop over every statement
  for (int i = 0; i < block->child_count; i++) {

    ASTNode stmt = block->child_arr[i];

    // This assures us that at least this return statement will be reached
    // It doesn't HAVE to be this statement, this is just insurance
    if (stmt->node_type == NODE_RETURN) {
      return true;

    }
    // Check returns in if statement
    else if (stmt->node_type == NODE_IF) {

      ASTNode then_block = stmt->child_arr[I_IB1];
      ASTNode else_block = stmt->child_arr[I_IB2];
      bool both_branches_end = (block_ends_with_return(then_block) &&
                                block_ends_with_return(else_block));

      // Return is guaranteed ONLY if both branches end with return
      if (both_branches_end == true) {
        return true;
      }
    }
  }

  // No guaranteed return stmt found
  return false;
}

static ErrCode resolve_while(ASTNode while_stmt, SymbTables *st) {

  // Assert node and define err
  assert(while_stmt != NULL);
  assert(while_stmt->node_type == NODE_WHILE);
  assert(while_stmt->data_type == DT_UNIT);
  ErrCode err = SUCCESS;

  // Set child nodes
  ASTNode cond = while_stmt->child_arr[I_COND];
  ASTNode block = while_stmt->child_arr[I_WB];

  // Condition
  err = resolve_condition(cond, st);
  RETURN_ON_ERROR(err, "inside while condition");

  // Caller deals with scopes
  err = ST_enter_scope(st);
  RETURN_ON_ERROR(err, "inside while, on entering scope");

  // Block
  err = resolve_block(block, st, false);
  RETURN_ON_ERROR(err, "inside while block");

  // After entering scope, exit it
  ST_exit_scope(st);

  return SUCCESS;
}

static ErrCode resolve_return(ASTNode return_stmt, SymbTables *st) {

  // Assert node and define err
  assert(return_stmt != NULL);
  assert(return_stmt->node_type == NODE_RETURN);
  ErrCode err = SUCCESS;

  return SUCCESS;
}

ErrCode define_variable_symbol(ASTNode idtf, SymbType sb_type, SymbTables *st) {

  assert(idtf != NULL);
  assert(idtf->node_type == NODE_IDENTIFIER);
  // Intended only for variables
  assert(sb_type == SB_VAL || SB_VAR);

  // Make draft
  SymbDraft symb_draft = {
      .identifier = idtf->data.string_val,
      .symb_type = sb_type,
      .data_type = idtf->data_type,
  };

  // Try defining it - this also checks redefinition!
  ErrCode err = ST_define_symbol(st, &symb_draft);
  RETURN_ON_ERROR(err, "failed defining symbol");

  // Save symbol pointer
  Symbol *local_symb = ST_get_symbol(st, idtf->data.string_val);
  assert(local_symb != NULL);
  idtf->symbol_ptr = local_symb;

  // Symbol defined
  return SUCCESS;
}

void AST_destroy_tree(ASTNode root) {

  if (root == NULL)
    return;

  for (int i = 0; i < root->child_count; i++) {
    // recursively destroy every child xd
    AST_destroy_tree(root->child_arr[i]);
  }

  // Free child pointer array
  if (root->child_arr != NULL) {
    free(root->child_arr);
  }

  // Free string data
  if (root->node_type == NODE_IDENTIFIER) {
    free(root->data.string_val);
  } else if (root->node_type == NODE_LITERAL && root->data_type == DT_STRING) {
    free(root->data.string_val);
  }

  // Finally,
  free(root);
}
