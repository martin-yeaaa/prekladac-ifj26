// Project: prekladac-ifj26
// File: ast.c
// Author: Martin Vaško (vaskom00)

// Implementation of abstract syntax tree = AST

#include "ast.h"
#include "symtable.h"
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
 * @return `0` on success, `99` on memory allocation failure,
 * `6` if conversion fails.
 */
static int set_integer_node_data(ASTNode node, char *data_string);

/**
 * @brief Converts string to 64 bit float and sets it as node data.
 * @param node Node into which data will be inserted. Not `NULL`.
 * @param data_string Data to be converted and saved. Not `NULL`.
 * @return `0` on success, `99` on memory allocation failure,
 * `6` if conversion fails.
 */
static int set_double_node_data(ASTNode node, char *data_string);

/**
 * @brief Copies string into node data.
 * @param node Node into which data will be inserted. Not `NULL`.
 * @param data_string Data to be copied. Not `NULL`.
 * @return `0` on success, `99` on memory allocation failure,
 */
static int set_string_node_data(ASTNode node, char *data_string);

/**
 * @brief Checks whether the node should hold symbol identifier.
 * @param node Node to check. Must not be `NULL`
 * @return `true` if node should hold identifier, `false` otherwise.
 */
static bool node_has_identifier(ASTNode node);

// Function definitions

int AST_init_node(ASTNode *node, NodeType node_type, DataType data_type,
                  char *node_data) {

  // Variable that temporarily holds error codes
  int err;

  // Allocate memory for new node
  ASTNode new_node = malloc(sizeof(struct ast_node_struct));
  // Check success
  if (new_node == NULL)
    return 99;

  // Initialize node
  // These types MUST be initialized first!
  new_node->node_type = node_type;
  new_node->data_type = data_type;

  // Figure out type of data node_data is, and assign it
  if (node_has_identifier(new_node)) {

    // Data is identifier
    err = set_string_node_data(new_node, node_data);
    if (err != 0)
      return err; // Check for errors
  }
  // Check data types: Integer
  else if (data_type == DT_INTEGER) {
    err = set_integer_node_data(new_node, node_data);
    if (err != 0)
      return err; // Check for errors
  }
  // Double
  else if (data_type == DT_DOUBLE) {
    err = set_double_node_data(new_node, node_data);
    if (err != 0)
      return err; // Check for errors
  }
  // String
  else if (data_type == DT_STRING) {
    err = set_string_node_data(new_node, node_data);
    if (err != 0)
      return err; // Check for errors
  } else {
    // Placeholder
    new_node->data.string_val = NULL;
  }

  // These values might be modified later
  new_node->symbol_ptr = NULL;
  new_node->child_node_count = 0;
  new_node->child_node_arr = NULL;

  // Don't forget to set the node!
  *node = new_node;

  return 0;
}

static bool node_has_identifier(ASTNode node) {

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
    ret = true;
    break;

  default:
    ret = false;
    break;
  }

  return ret;
}

static int set_integer_node_data(ASTNode node, char *data_string) {

  assert(node != NULL);
  assert(data_string != NULL);

  // Pointers that help check correct conversion
  char *string_end = data_string[strlen(data_string)];
  char *conv_end = NULL;

  // Conversion to long long
  long long int_data = strtoll(data_string, &conv_end, 10);

  // Check errors
  // Value out of range, and narrowing long long to 32 bit int
  if (errno == ERANGE || int_data > INT32_MAX || int_data < INT32_MIN) {
    errno = 0; // Reset errno
    return 6;
  }
  // String wasn't fully converted
  if (string_end != conv_end) {
    return 6;
  }

  // Assign data
  node->data.int_val = int_data;

  return 0;
}

static int set_double_node_data(ASTNode node, char *data_string) {

  assert(node != NULL);
  assert(data_string != NULL);

  // Pointers that help check correct conversion
  char *string_end = data_string[strlen(data_string)];
  char *conv_end = NULL;

  // Conversion
  double dbl_data = strtod(data_string, &conv_end);

  // Check errors
  // Value out of range
  if (errno == ERANGE) {
    errno = 0; // Reset errno
    return 6;
  }
  // String wasn't fully converted
  if (string_end != conv_end) {
    return 6;
  }

  // Assign data
  node->data.double_val = dbl_data;

  return 0;
}

static int set_string_node_data(ASTNode node, char *data_string) {

  assert(node != NULL);
  assert(data_string != NULL);

  // node_data should be symbol identifier
  node->data.string_val = malloc(sizeof(char) * (strlen(data_string) + 1));
  // Check success
  if (node->data.string_val == NULL) {
    return 99; // Memory error
  }
  // Copy string
  strcpy(node->data.string_val, data_string);

  return 0;
}

int AST_make_tree(ASTNode parent, int child_count, ASTNode *child_arr);
