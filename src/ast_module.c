// Project: prekladac-ifj26
// File: ast_module.c
// Author: Martin Vaško (xvaskom00)

// Implementation of abstract syntax tree = AST functions

#include "ast_module.h"
#include "error_codes.h"
#include <assert.h>
#include <errno.h>  // For conversion errors
#include <stdint.h> // Integer limits
#include <stdlib.h>
#include <string.h>

// Private functions declarations

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

// Function implementations

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
