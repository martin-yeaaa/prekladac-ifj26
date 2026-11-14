// Project: prekladac-ifj26
// File: ast.c
// Author: Martin Vaško (vaskom00)

// Implementation of abstract syntax tree = AST

#include "ast.h"
#include "symtable.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Declarations of internal functions

/**
 * @brief Converts string to 32 bit int and sets it as node data.
 * @param node Node into which data will be inserted.
 * @param data_string Data to be converted and saved.
 * @return `0` on success, `99` on memory allocation failure,
 * `6` if conversion fails.
 */
static int set_integer_node_data(ASTNode node, char *data_string);

static int set_double_node_data();
static int set_string_node_data();

int AST_init_node(ASTNode *node, NodeType node_type, DataType data_type,
                  char *node_data) {

  // Allocate memory for new node
  ASTNode new_node = malloc(sizeof(struct ast_node_struct));
  // Check success
  if (new_node == NULL)
    return NULL;

  // Initialize node
  new_node->node_type = node_type;
  new_node->data_type = data_type;

  // Figure out type of data, and assign it
  switch (node_type) {

  // If node should contain symbol
  case NODE_IDENTIFIER:
  case NODE_ASSIGNMENT:
  case NODE_VAR_DEF:
  case NODE_VAL_DEF:
  case NODE_FUNC_DEF:
  case NODE_MAIN_FUNC_DEF:

    // node_data should be symbol identifier
    new_node->data.string_val = malloc(sizeof(char) * (strlen(node_data) + 1));
    // Check success
    if (new_node->data.string_val == NULL) {
      free(new_node);
      return NULL;
    }
    // Copy string
    strcpy(new_node->data.string_val, node_data);
    break;

  case NODE_LITERAL:
    if (data_type == DT_INTEGER) {
      new_node->data.int_val = 4; // this will work trust
    }

  default:
    new_node->data.bool_val = false;
    break;
  }
  // this doesnt work lmao new_node->data = node_data;

  // These might be modified later
  new_node->symbol_ptr = NULL;
  new_node->child_node_count = 0;
  new_node->child_node_arr = NULL;

  *node = new_node;
  // return new_node;
}

int AST_make_tree(ASTNode parent, int child_count, ASTNode *child_arr);
