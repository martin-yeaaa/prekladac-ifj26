// Project: prekladac-ifj26
// File: ast.c
// Author: Martin Vaško (vaskom00)

// Implementation of abstract syntax tree = AST and semantic analysis functions

#include "semantics.h"
#include "symtable.h"
#include "error_codes.h"
#include <assert.h>
#include <errno.h> // For conversion errors
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Declarations of internal functions

/**
 * @brief Converts string to 32 bit int and sets it as node data.
 * @param node Node into which data will be inserted. Not `NULL`.
 * @param data_string Data to be converted and saved. Not `NULL`.
 * @return `SUCCESS` on success,
 * `ERR_INCOMPATIBLE_TYPE` if conversion fails.
 */
static ErrCode set_integer_node_data(ASTNode node, char *data_string);

/**
 * @brief Converts string to 64 bit float and sets it as node data.
 * @param node Node into which data will be inserted. Not `NULL`.
 * @param data_string Data to be converted and saved. Not `NULL`.
 * @return `SUCCESS` on success,
 * `ERR_INCOMPATIBLE_TYPE` if conversion fails.
 */
static ErrCode set_double_node_data(ASTNode node, char *data_string);

/**
 * @brief Copies string into node data.
 * @param node Node into which data will be inserted. Not `NULL`.
 * @param data_string Data to be copied. Not `NULL`.
 * @return `SUCCESS` on success,
 * `ERR_INTERNAL` on memory allocation failure,
 */
static ErrCode set_string_node_data(ASTNode node, char *data_string);

/** TODO delete probably
 * @brief Checks whether the node should hold symbol identifier.
 * @param node Node to check. Must not be `NULL`
 * @return `true` if node should hold identifier, `false` otherwise.

static bool node_has_identifier(ASTNode node);
*/

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
  new_node->child_node_count = 0;
  new_node->child_node_arr = NULL;

  // Don't forget to set the node!
  *node = new_node;

  return SUCCESS;
}

/* TODO probably delete
static bool node_has_identifier(ASTNode node) {

  assert(node != NULL);

  // Return value
  bool ret = false;

  // Result depends on node type
  switch (node->node_type) {

  // Node should contain identifier if it's one of these node types:
  case NODE_IDENTIFIER:
  case NODE_ASSIGNMENT:
  case NODE_VAR_DEF:
  case NODE_VAL_DEF:
  case NODE_FUNC_DEF:
  case NODE_MAIN_FUNC_DEF:
  case NODE_FUNC_CALL:
    ret = true;
    break;

  default:
    ret = false;
    break;
  }

  return ret;
}
  */

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

ErrCode AST_make_tree(ASTNode parent, int child_count, ASTNode *child_arr) {

  // Assertions
  assert(parent != NULL);
  assert(child_count > 0);
  assert(child_arr != NULL);

  // Update parent
  parent->child_node_count = child_count;

  // Create array of pointers to child nodes
  parent->child_node_arr = malloc(sizeof(ASTNode) * child_count);
  if (parent->child_node_arr == NULL) {
    return ERR_INTERNAL;
  }

  // Fill array
  for (int i = 0; i < child_count; i++) {
    parent->child_node_arr[i] = child_arr[i];
  }

  // Success
  return SUCCESS;
}
