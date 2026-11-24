// Project: prekladac-ifj26
// File: semantics.c
// Author: Martin Vaško (xvaskom00)

// Implementation of semantic analysis functions

#include "semantics.h"
#include "semantics_priv.h"

#include "ast_module.h" // Creates node int to double
#include "bst.h"        // For recursion check
#include "error_codes.h"
#include "symtable.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Variable that holds currently resolved function definition.
// It's only used to simplify comparing data types of functions
// and their return statements.
static ASTNode current_func_def;

// Variable that holds all function definition nodes. Used for looking up func
// def nodes using their identifier strings.
static BSTNode func_def_list;

// Function definitions

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

static ErrCode check_other_semantics(ASTNode root) {

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

static int find_func_calls(ASTNode func_body, BSTNode *call_list) {

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

static ErrCode define_func_symbols(ASTNode root, SymbTables *st) {

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
    bool success_ins = BST_insert(&func_def_list, func_idtf->data.string_val,
                                  &func_def, sizeof(ASTNode));
    if (success_ins == false) {
      RETURN_ON_ERROR(ERR_INTERNAL,
                      "failed to insert function definition into list.");
    }

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

static ErrCode infer_or_check_types(ASTNode lhs, ASTNode rhs, ASTNode parent) {

  // Assert nodes and define err
  assert(lhs != NULL);
  assert(lhs->symbol_ptr != NULL);
  // Left side must be identifier
  assert(lhs->node_type == NODE_IDENTIFIER);

  // Right side must have known type
  assert(rhs != NULL);
  assert(rhs->data_type != DT_UNDEF);

  ErrCode err = SUCCESS;

  // Only 2 possibilities

  // 1 - lhs type is undefined -> infer type from right side
  if (lhs->data_type == DT_UNDEF) {
    lhs->data_type = rhs->data_type;
    // Update symbol inside symbol table
    lhs->symbol_ptr->data_type = rhs->data_type;
  }
  // 2 - lhs type is defined -> compare both sides
  else {

    if (lhs->data_type != rhs->data_type) {
      // Check if conversion is possible
      if (lhs->data_type == DT_DOUBLE && rhs->data_type == DT_INTEGER) {
        // Convert rhs to double
        err = convert_int_to_double(rhs, parent);
        RETURN_ON_ERROR(err, "implicit conversion int to double failed.");

      } else {
        RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE,
                        "type incompatibility between rhs and lhs.");
      }
    }
  }

  return SUCCESS;
}

static ErrCode convert_int_to_double(ASTNode child, ASTNode parent) {

  // Assert node and define err
  assert(child != NULL);
  assert(child->data_type == DT_INTEGER);
  assert(parent != NULL);
  ErrCode err = SUCCESS;

  // Create new node
  ASTNode conversion;
  err = AST_init_node(&conversion, NODE_INT_TO_DBL, DT_DOUBLE, NULL);
  RETURN_ON_ERROR(err, "conversion failed when initializing node.");

  int child_idx = -1;
  // Find child
  for (int i = 0; i < parent->child_count; i++) {
    if (parent->child_arr[i] == child) {
      child_idx = i;
      break;
    }
  }
  assert(child_idx != -1);

  // Link conversion node to child
  err = AST_node_add_child(conversion, child);
  if (err != SUCCESS) {
    free(conversion);
    RETURN_ON_ERROR(err,
                    "conversion failed when adding child to conversion node.");
  }

  // Relink parent to conversion node
  parent->child_arr[child_idx] = conversion;

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

static ErrCode resolve_any_subexpr(ASTNode subexpr, SymbTables *st) {

  // Assert node and define err
  assert(subexpr != NULL);
  ErrCode err = SUCCESS;

  // Find appropriate resolve function
  switch (subexpr->node_type) {

  case NODE_ADD:
  case NODE_SUB:
  case NODE_MUL:
  case NODE_DIV: {
    err = resolve_binary_op(subexpr, st);
    break;
  }
  case NODE_UN_PLUS:
  case NODE_UN_MINUS: {
    err = resolve_unary_op(subexpr, st);
    break;
  }
  case NODE_IDENTIFIER: {
    err = resolve_rhs_var_id(subexpr, st);
    break;
  }
  case NODE_LITERAL: {
    // We know everything there is about literals
    // Nothing to do
    break;
  }
  // If no function was matched, it's my fault
  default: {
    fprintf(stderr, "Failed matching resolve func for subexpression.\n");
    err = ERR_INTERNAL;
    break;
  }
  }

  // Returns SUCCESS or error code
  return err;
}

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
  if (func_id->data_type != DT_UNDEF && func_id->data_type != DT_UNIT &&
      func_body->node_type == NODE_BLOCK &&
      func_body->child_arr[last]->node_type == NODE_IF) {

    // DON'T CHECK TYPES, they are checked in return statements.

  } else {
    // Check types of both sides, or infer type
    err = infer_or_check_types(func_id, func_body, func_def);
    RETURN_ON_ERROR(err, "type error in func_def");
    // func body might have changed
    func_body = func_def->child_arr[I_FBODY];
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
  // Also check if it's a function and not a variable
  if (func_symb->symb_type == SB_VAL || func_symb->symb_type == SB_VAR) {
    RETURN_ON_ERROR(ERR_OTHER_SEMANTIC,
                    "variable identifier in place of a function!");
  }

  // If function call has undefined data type, we simply resolve it first!
  if (func_symb->data_type == DT_UNDEF) {

    // Save scope
    // This also resets scope back to global scope
    LNode local_stack = ST_save_scope_stack(st);

    // Get func def node to resolve
    ASTNode *unresolved_ptr =
        BST_get_data(func_def_list, func_id->data.string_val);
    assert(unresolved_ptr != NULL); // This cannot fail!
    // BST_get_data returns pointer to our data, need to dereference
    ASTNode unresolved = *unresolved_ptr;
    assert(unresolved != NULL);

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
  // Also save symbol ptr
  func_id->symbol_ptr = func_symb;

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
    err = resolve_arg(arg, st);
    RETURN_ON_ERROR(err, "inside function call in argument");

    assert(arg->data_type != DT_UNDEF);

    // Check if it matches param type
    DataType arg_type = arg->data_type;
    DataType param_type = func_symb->param_type_arr[a];

    // Some built in functions accept any data type
    if (param_type == DT_ANY) {
      // Don't check parameter type
      continue;
    }
    if (arg_type != param_type) {

      // Check implicit conversion
      if (arg_type == DT_INTEGER && param_type == DT_DOUBLE) {

        err = convert_int_to_double(arg, func_arg);
        RETURN_ON_ERROR(err, "conversion failed inside argument");
      } else {
        RETURN_ON_ERROR(ERR_FUN_ARGS,
                        "function called with wrong data type of argument");
      }
    }
    // else data types are equal
  }

  return SUCCESS;
}

static ErrCode resolve_arg(ASTNode arg, SymbTables *st) {

  // Assert node and define err
  assert(arg != NULL);
  ErrCode err = SUCCESS;

  // Resolve appropriately
  switch (arg->node_type) {

  case NODE_UN_MINUS:
  case NODE_UN_PLUS: {

    // In this case, unary operator can only be used on a literal
    // Unary operator with identifier is an expression!
    assert(arg->child_count == 1);
    ASTNode literal = arg->child_arr[I_OP];
    assert(literal->node_type == NODE_LITERAL);
    assert(literal->data_type != DT_UNDEF);

    // Unary operator can only be used on a number value
    if (literal->data_type != DT_INTEGER && literal->data_type != DT_DOUBLE) {
      RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE,
                      "unary operator can only be used on a number");
    }

    // Propagate data type
    arg->data_type = literal->data_type;
    break;
  }
  case NODE_IDENTIFIER: {

    err = resolve_rhs_var_id(arg, st);
    break;
  }
  case NODE_LITERAL: {
    // We know everything there is about literals
    // Nothing to do
    break;
  }

  // If no type was matched, it's my fault
  default: {
    fprintf(stderr, "Failed matching resolve func for term.\n");
    err = ERR_INTERNAL;
    break;
  }
  }

  return err;
}

static ErrCode resolve_expression(ASTNode expr, SymbTables *st) {

  // Assert node and define err
  assert(expr != NULL);
  assert(expr->node_type == NODE_EXPR);
  ErrCode err = SUCCESS;

  ASTNode subexpr = expr->child_arr[I_OP];
  err = resolve_any_subexpr(subexpr, st);
  RETURN_ON_ERROR(err, "inside expression");

  // Propagate data type
  expr->data_type = subexpr->data_type;

  return SUCCESS;
}

static ErrCode resolve_var_def(ASTNode var_def, SymbTables *st) {

  // Assert node and define err
  assert(var_def != NULL);
  assert(var_def->node_type == NODE_VAR_DEF);
  assert(var_def->data_type == DT_UNIT);
  ErrCode err = SUCCESS;

  // Resolve right side of definition first
  ASTNode rhs_node = var_def->child_arr[I_RHS];
  err = resolve_any_rhs(rhs_node, st);
  RETURN_ON_ERROR(err, "inside var_def");

  // Define symbol second
  ASTNode var_def_id = var_def->child_arr[I_LHS];
  err = define_variable_symbol(var_def_id, SB_VAR, st);
  RETURN_ON_ERROR(err, "inside var_def");

  // Do type check
  err = infer_or_check_types(var_def_id, rhs_node, var_def);
  RETURN_ON_ERROR(err, "inside var_def");

  return SUCCESS;
}

static ErrCode resolve_val_def(ASTNode val_def, SymbTables *st) {

  // Assert node and define err
  assert(val_def != NULL);
  assert(val_def->node_type == NODE_VAL_DEF);
  assert(val_def->data_type == DT_UNIT);
  ErrCode err = SUCCESS;

  // Resolve right side of definition first
  ASTNode rhs_node = val_def->child_arr[I_RHS];
  err = resolve_any_rhs(rhs_node, st);
  RETURN_ON_ERROR(err, "inside val_def");

  // Define symbol second
  ASTNode val_def_id = val_def->child_arr[I_LHS];
  err = define_variable_symbol(val_def_id, SB_VAL, st);
  RETURN_ON_ERROR(err, "inside val_def");

  // Do type check
  err = infer_or_check_types(val_def_id, rhs_node, val_def);
  RETURN_ON_ERROR(err, "inside val_def");

  return SUCCESS;
}

static ErrCode resolve_assignment(ASTNode assign, SymbTables *st) {

  // Assert node and define err
  assert(assign != NULL);
  assert(assign->node_type == NODE_ASSIGNMENT);
  assert(assign->data_type == DT_UNIT);
  ErrCode err = SUCCESS;

  // Get symbol
  ASTNode assign_id = assign->child_arr[I_LHS];
  Symbol *assign_symb = ST_get_symbol(st, assign_id->data.string_val);

  // Check symbol validity
  if (assign_symb == NULL) {
    RETURN_ON_ERROR(ERR_UNDEF_SYM, "assignment to undefined variable.");
  }
  if (assign_symb->symb_type == SB_VAL) {
    RETURN_ON_ERROR(ERR_INVALID_ASSIGN, "assignment to unmodifiable variable.");
  }
  // Assignment can only happen to var
  else if (assign_symb->symb_type != SB_VAR) {
    RETURN_ON_ERROR(ERR_INVALID_ASSIGN, "assignment to function identifier!");
  }
  // Symbol is valid, write it into assign_id
  assign_id->symbol_ptr = assign_symb;
  // Also set the data type
  assign_id->data_type = assign_symb->data_type;

  // Resolve right side of definition
  ASTNode rhs_node = assign->child_arr[I_RHS];
  err = resolve_any_rhs(rhs_node, st);
  RETURN_ON_ERROR(err, "resolving rhs inside assignment");

  // Do type check
  err = infer_or_check_types(assign_id, rhs_node, assign);
  RETURN_ON_ERROR(err, "checking types inside assignment");

  return SUCCESS;
}

static ErrCode resolve_binary_op(ASTNode bin_op, SymbTables *st) {

  // Assert node and define err
  assert(bin_op != NULL);
  assert(bin_op->node_type == NODE_ADD || bin_op->node_type == NODE_SUB ||
         bin_op->node_type == NODE_MUL || bin_op->node_type == NODE_DIV);
  ErrCode err = SUCCESS;

  // Basically copied code from resolve_condition but whateverrr
  // Also there ARE differences

  ASTNode left_subexpr = bin_op->child_arr[I_L_O];
  ASTNode right_subexpr = bin_op->child_arr[I_R_O];

  // Resolve both sides
  err = resolve_any_subexpr(left_subexpr, st);
  RETURN_ON_ERROR(err, "inside expression, left side");
  err = resolve_any_subexpr(right_subexpr, st);
  RETURN_ON_ERROR(err, "inside expression, right side");

  // Data types
  DataType left_dt = left_subexpr->data_type;
  DataType right_dt = right_subexpr->data_type;
  assert(left_dt != DT_UNDEF);
  assert(right_dt != DT_UNDEF);

  // Check types:
  // Unit types cannot be operands
  if (left_dt == DT_UNIT || right_dt == DT_UNIT) {
    RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE,
                    "Unit type cannot be used in operations in expression.");
  }
  // The only allowed operator for strings is +
  if (left_dt == DT_STRING && right_dt == DT_STRING) {
    if (bin_op->node_type != NODE_ADD) {
      RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE, "invalid string operation.");
    }
  }

  // Check type compatibility
  if (left_dt != right_dt) {

    // Check for possible implicit conversion
    if (left_dt == DT_DOUBLE && right_dt == DT_INTEGER) {

      // Convert right side to double
      err = convert_int_to_double(right_subexpr, bin_op);
      RETURN_ON_ERROR(err, "conversion of right expression failed");

    } else if (left_dt == DT_INTEGER && right_dt == DT_DOUBLE) {

      // Convert left side to double
      err = convert_int_to_double(left_subexpr, bin_op);
      RETURN_ON_ERROR(err, "conversion of left expression failed");
    } else {
      // Type incompatibility
      RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE,
                      "incompatible types in expression.");
    }
  }
  // Else types are compatible

  // Conversion creates new AST nodes, we need to update our variables
  left_subexpr = bin_op->child_arr[I_L_O];
  right_subexpr = bin_op->child_arr[I_R_O];

  // Propagate type
  assert(left_subexpr->data_type == right_subexpr->data_type);
  bin_op->data_type = left_subexpr->data_type;

  return SUCCESS;
}

static ErrCode resolve_unary_op(ASTNode un_op, SymbTables *st) {

  // Assert node and define err
  assert(un_op != NULL);
  assert(un_op->node_type == NODE_UN_PLUS || un_op->node_type == NODE_UN_MINUS);
  ErrCode err = SUCCESS;

  // Resolve child
  ASTNode subexpr = un_op->child_arr[I_UN];
  err = resolve_any_subexpr(subexpr, st);
  RETURN_ON_ERROR(err, "inside unary operator");

  // Check if child has valid type
  assert(subexpr->data_type != DT_UNDEF);
  if (subexpr->data_type == DT_UNIT || subexpr->data_type == DT_STRING) {
    RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE,
                    "unary operator can only be used on number values.");
  }

  // Propagate type
  un_op->data_type = subexpr->data_type;

  return SUCCESS;
}

// This is intended only for variable identifiers in expressions and arguments
static ErrCode resolve_rhs_var_id(ASTNode idtf, SymbTables *st) {

  // Assert node and define err
  assert(idtf != NULL);
  assert(idtf->node_type == NODE_IDENTIFIER);

  Symbol *var_symb = ST_get_symbol(st, idtf->data.string_val);
  // Variable must be defined by now
  if (var_symb == NULL) {
    RETURN_ON_ERROR(ERR_UNDEF_SYM, "undefined variable.");
  }
  // Also check if it's a variable and not a function
  if (var_symb->symb_type == SB_FUNC || var_symb->symb_type == SB_MAIN_FUNC ||
      var_symb->symb_type == SB_BUILTIN) {
    RETURN_ON_ERROR(ERR_OTHER_SEMANTIC,
                    "function identifier in place of a variable!");
  }

  // Data type should be defined with the symbol
  assert(var_symb->data_type != DT_UNDEF);

  // Infer data type of symbol
  idtf->data_type = var_symb->data_type;
  // Also set symbol ptr
  idtf->symbol_ptr = var_symb;

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
  DataType right_dt = right_expr->data_type;
  assert(left_dt != DT_UNDEF);
  assert(right_dt != DT_UNDEF);

  // Operators == and != work with every type
  // Other operators do NOT work with Unit type
  if (rel_op->node_type != NODE_EQ && rel_op->node_type != NODE_NOT_EQ) {
    if (left_dt == DT_UNIT || right_dt == DT_UNIT) {
      RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE,
                      "inside condition, cannot compare Unit data type.");
    }
  }

  // Check types
  if (left_dt != right_dt) {

    // Check for possible implicit conversion
    if (left_dt == DT_DOUBLE && right_dt == DT_INTEGER) {

      // Convert right side to double
      err = convert_int_to_double(right_expr, rel_op);
      RETURN_ON_ERROR(err, "conversion of right expression failed");

    } else if (left_dt == DT_INTEGER && right_dt == DT_DOUBLE) {

      // Convert left side to double
      err = convert_int_to_double(left_expr, rel_op);
      RETURN_ON_ERROR(err, "conversion of left expression failed");
    } else {
      // Type incompatibility
      RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE,
                      "incompatible types in condition.");
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

  // Resolve statement
  ASTNode rhs = return_stmt->child_arr[I_RET];
  err = resolve_any_rhs(rhs, st);
  RETURN_ON_ERROR(err, "inside return statement.");
  assert(rhs->data_type != DT_UNDEF);

  // Check types against currently resolved function
  assert(current_func_def != NULL);
  DataType func_dt = current_func_def->data_type;
  DataType return_dt = rhs->data_type;

  // Compare type with function definition
  // It must be explicitly declared!
  assert(func_dt != DT_UNDEF);

  if (func_dt != return_dt) {

    // Check if conversion can happen
    if (func_dt == DT_DOUBLE && return_dt == DT_INTEGER) {

      err = convert_int_to_double(rhs, return_stmt);
      RETURN_ON_ERROR(err, "conversion failed inside return.");
      // Update rhs after conversion
      rhs = return_stmt->child_arr[I_RET];
    }
    // Conversion cannot happen -> incompatible types
    else {
      RETURN_ON_ERROR(ERR_INCOMPATIBLE_TYPE,
                      "incompatible type in return statement.");
    }
  }

  // Propagate type
  return_stmt->data_type = rhs->data_type;

  return SUCCESS;
}

static ErrCode define_variable_symbol(ASTNode idtf, SymbType sb_type,
                                      SymbTables *st) {

  assert(idtf != NULL);
  assert(idtf->node_type == NODE_IDENTIFIER);
  // Intended only for variables
  assert(sb_type == SB_VAL || sb_type == SB_VAR);

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
