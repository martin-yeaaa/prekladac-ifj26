// bst.c
// Authors: Martin Vaško (vaskom00)

// Implementation of height-balanced binary search tree

#include "bst.h"
#include "bst_priv.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool error_flag = false;

BSTNode BST_new() {

  // Empty tree is simply NULL pointer
  return NULL;
}

bool BST_insert(BSTNode root, BSTKey key, BSTData data) {

  // Key must not be NULL
  assert(key != NULL);

  // Call function for recursive insertion, sets error_flag
  root = insert_recursive(root, key, data);

  if (error_flag == true) {
    fprintf(stderr, "very bad\n");
    return false;
  } else {
    return true;
  }
}

void BST_delete(BSTNode root, BSTKey key) {

  // Key must not be NULL
  assert(root != NULL);
  assert(key != NULL);
}

BSTData BST_get_data(BSTNode root, BSTKey key) {

  // Parameters must not be NULL
  assert(root != NULL);
  assert(key != NULL);

  return NULL;
}

BSTNode insert_recursive(BSTNode root, BSTKey key, BSTData data) {

  // Empty tree
  if (root == NULL) {

    root = create_node(key, data);

    // Check if creation succeeded
    if (root == NULL) {
      error_flag = true; // Creation failed, set error_flag
    }

  // Not empty tree
  } else {

    // When comparing keys, first parameter is the searched key,
    // second parameter is node key
    int key_comparison = compare_keys(key, root->key);

    // Exact match -> rewrite data
    if (key_comparison == 0) {
      root->data = data;
    }

    // less than 0 -> left son
    else if (key_comparison < 0) {
      root->left = insert_recursive(root->left, key, data);
    }
    // more than 0 -> right son
    else { // key_comparison > 0
      root->right = insert_recursive(root->right, key, data);
    }
  }

  // TODO update_height()
  // TODO balance()

  return root;
}

int compare_keys(BSTKey searched_key, BSTKey node_key) {
  // Parameters must not be NULL
  assert(searched_key != NULL);
  assert(node_key != NULL);

  // Lexicographical comparison
  return strcmp(searched_key, node_key);
}

BSTNode find_closest_node(BSTNode node, BSTKey key) {

  // If *node points to a tree, it must not be an empty tree
  if (node != NULL)
    assert(is_empty(node) == false);
  // key must not be NULL
  assert(key != NULL);
}

BSTNode create_node(BSTKey key, BSTData data) {

  // Parameters must not be NULL
  assert(key != NULL);
  assert(data != NULL);
}