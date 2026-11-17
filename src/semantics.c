// Project: prekladac-ifj26
// File: semantics.c
// Author: Martin Vaško (vaskom00)

// Implementation of abstract syntax tree = AST and semantic analysis functions

#include "semantics.h"
#include "error_codes.h"
#include "semantics_priv.h"
#include "symtable.h"

#include <assert.h>
#include <errno.h> // For conversion errors
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macro for checking for errors and returning if error occured
#define RETURN_ON_ERROR(error_code, error_msg)                                 \
  do {                                                                         \
    if (error_code != SUCCESS) {                                               \
      fprintf(stderr, "Error: %s\n", error_msg);                               \
      return error_code;                                                       \
    }                                                                          \
  } while (0)

// Function definitions

ErrCode AST_init_node(ASTNode *node, NodeType node_type, DataType data_type,
                      char *node_data) {

  // Variable that temporarily holds error codes
  ErrCode err = 0;

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

  // Update parent count
  parent->child_count += 1;

  // Dynamic array allocation
  if (parent->child_limit == 0) {
    // First child node
    parent->child_limit = 2; // 2 is a common amount of AST child nodes
    parent->child_arr = malloc(sizeof(ASTNode) * parent->child_limit);
    if (parent->child_arr == NULL) {
      return ERR_INTERNAL;
    }

  } else if (parent->child_limit < parent->child_count) {
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

  // After making room, assign child node
  parent->child_arr[i] = child;

  // Success
  return SUCCESS;
}

ErrCode semantic_analysis(ASTNode root, SymbTables *st) {

  // Error code return variable
  ErrCode err = SUCCESS;

  // Check return statements, now we can already tell
  // if function type was explicitly declared!
  err = check_return_stmts(root);
  RETURN_ON_ERROR(err, "found invalid return statement.");

  // Initialize symbol table
  err = ST_init(st);
  RETURN_ON_ERROR(err, "failed to initialize symbol table.");

  // Fill in symbol table with function declarations
  err = define_func_symbols(root, st);
  RETURN_ON_ERROR(err, "failed defining function symbols.");

  // Fill in symbol table with local declarations (variables)
  err = define_local_symbols(root, st);
  RETURN_ON_ERROR(err, "failed defining symbols of variables.");

  // Helper variable
  bool type_update;

  // Resolve as many data types as possible
  do {
    type_update = false;

    /*
    // 3 phases - resolve data types of:
    // 1 - expressions
    err = resolve_expr_types(root, st, &type_update);
    if (err != SUCCESS)
      return err;

    // 2 - assignments and definitions of variables
    err = resolve_assignment_types(root, st, &type_update);
    if (err != SUCCESS)
      return err;

    // 3 - function return values
    err = resolve_func_types(root, st, &type_update);
    if (err != SUCCESS)
      return err;
      */

    // Might be a good idea to combine them, so we don't walk the tree so much
    err = resolve_data_types(root, st, &type_update);
    RETURN_ON_ERROR(err, "data type check failed.");

    // If a new type was resolved, repeat
  } while (type_update == true);

  // Check something else? TODO

  // Types are checked, symbols are defined, we are DONE
  return SUCCESS;
}

/*
static ASTNode get_node_of_type(ASTNode node, NodeType searched_type) {

  assert(node != NULL);

  // Helper variable
  bool node_found = false;

  // Check every child, return first found
  for (int i = 0; i < node->child_count; i++) {

    if (node->child_arr[i]->node_type == searched_type) {
      return node->child_arr[i];
    }
  }

  // Node not found
  return NULL;
}
*/

ErrCode check_return_stmts(ASTNode root) {

  assert(root != NULL);

  // Direct children of root are only function definitions
  for (int i = 0; i < root->child_count; i++) {

    // Get function def node
    ASTNode func_def = root->child_arr[i];

    // Assert correct AST structure
    assert(func_def->node_type == NODE_FUNC_DEF ||
           func_def->node_type == NODE_MAIN_FUNC_DEF);
    assert(func_def->child_count == 3);

    ASTNode block = func_def->child_arr[I_FBODY];

    // Only a block can hold return statement
    if (block->node_type == NODE_BLOCK) {

      // Data type isn't set explicitly -> return is forbidden
      if (func_def->data_type == DT_UNDEF) {
        if (contains_return(block))
          return ERR_OTHER_SEMANTIC;
      }

      // Otherwise data type is set explicitly and can contain return
    }
  }

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
      return contains_return(child->child_arr[I_IB1]) ||
             contains_return(child->child_arr[I_IB2]);

      // While
    } else if (child->node_type == NODE_WHILE) {
      // Search block inside loop
      return contains_return(child->child_arr[I_WB]);
    }
  }

  return false;
}
