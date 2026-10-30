// bst.c
// Authors: Martin Vaško (vaskom00)

// Implementation of height-balanced binary search tree

#include "bst.h"
#include "bst_priv.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// set to true if an error occurs, namely malloc() failure
static bool error_flag = false;

// When deleting a node with two sons, the node gets replaced with another node
// further down the tree, which actually gets deleted. When replacing, we can
// replace key with another key. However, key is a pointer to a string. Now we
// have the same key -> the same pointer in two places. When deleting the
// replacement node further down the tree, we cannot free the key, because it
// would break the node we replaced above. This variable lets us know if a
// replacement has happened, so we know whether to free the key string or not
static bool key_used_in_replacement = false;

BSTNode BST_new() {

  // Empty tree is simply NULL pointer
  return NULL;
}

bool BST_insert(BSTNode *root, BSTKey key, BSTData data) {

  // Key must not be NULL
  assert(key != NULL);

  // Call function for recursive insertion, sets error_flag
  *root = insert_recursive(*root, key, data);

  if (error_flag == true) {
    fprintf(stderr, "Error: Failed to insert new node!\n");
    return false;
  } else {
    return true;
  }
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

  // Set new height
  update_height(root);
  // Fix imbalance
  root = rebalance_tree(root);

  return root;
}

BSTNode create_node(BSTKey key, BSTData data) {

  // Key must not be NULL
  assert(key != NULL);

  // Allocate memory for new node
  BSTNode new_node = malloc(sizeof(struct bst_node)); //(BSTNode is a pointer!)
  // Check allocation success
  if (new_node == NULL)
    return NULL;

  // Allocate memory for key string
  char *key_copy = malloc(sizeof(char) * (strlen(key) + 1));
  // Check allocation success
  if (key_copy == NULL) {
    free(new_node);
    return NULL;
  }

  // Copy string
  strcpy(key_copy, key);

  // Initialize new node
  new_node->key = key_copy;
  new_node->data = data;
  new_node->height = 1;
  new_node->left = NULL;
  new_node->right = NULL;

  return new_node;
}

void BST_delete(BSTNode *root, BSTKey key) {

  // Key must not be NULL
  assert(key != NULL);

  // Deletion happens in recursive function.
  // I didn't want BST_delete() itself to be recursive,
  // because it would have to return BSTNode, which isn't
  // really a problem, but it's prettier if it doesn't return anything.
  *root = delete_recursive(*root, key);
}

BSTNode delete_recursive(BSTNode root, BSTKey key) {
  // Key must not be NULL
  assert(key != NULL);

  // Empty tree -> nothing to do
  if (root == NULL) {
    return NULL;
  }

  // When comparing keys, first parameter is the searched key,
  // second parameter is node key
  int key_comparison = compare_keys(key, root->key);

  // Exact match -> delete node
  if (key_comparison == 0) {
    root = delete_node(root); // May call delete_recursive()->indirect recursion
  }
  // less than 0 -> left son
  else if (key_comparison < 0) {
    root->left = delete_recursive(root->left, key);
  }
  // more than 0 -> right son
  else { // key_comparison > 0
    root->right = delete_recursive(root->right, key);
  }

  // Set new height
  update_height(root);
  // Fix imbalance
  root = rebalance_tree(root);

  return root;
}

BSTNode delete_node(BSTNode node) {

  // Node must not be NULL
  assert(node != NULL);

  // Free node key string memory, only when it's not used elsewhere
  if (key_used_in_replacement == false)
    free(node->key);

  key_used_in_replacement = false; // After checking, we can set it back

  // Terminal node without sons
  if (node->left == NULL && node->right == NULL) {
    free(node); // Free node memory
    return NULL;
  }
  // Has only right son
  else if (node->left == NULL && node->right != NULL) {
    free(node);         // Free node memory
    return node->right; // Return non-NULL son
  }
  // Has only left son
  else if (node->left != NULL && node->right == NULL) {
    free(node);        // Free node memory
    return node->left; // Return non-NULL son
  }
  // Has both sons - we need to replace the node
  else {

    // Our replacement is right-most son of left subtree
    BSTNode replacement = get_right_most_son(node->left);

    node->key = replacement->key;
    key_used_in_replacement = true; // So we know not to free(key)
    node->data = replacement->data;

    // INDIRECT RECURSION!! But we need to delete replacement from left subtree
    node->left = delete_recursive(node->left, replacement->key);
    // Don't forget to return the replacement
    return node;
  }
}

BSTNode get_right_most_son(BSTNode node) {

  // Node must not be NULL
  assert(node != NULL);

  if (node->right == NULL)
    return node;
  else
    return get_right_most_son(node->right);
}

BSTData BST_get_data(BSTNode root, BSTKey key) {

  // Key must not be NULL
  assert(key != NULL);

  // Key was never found
  if (root == NULL) {
    return NULL;
  }

  // When comparing keys, first parameter is the searched key,
  // second parameter is node key
  int key_comparison = compare_keys(key, root->key);

  // Exact match -> return data
  if (key_comparison == 0) {
    return root->data;
  }
  // less than 0 -> left son
  else if (key_comparison < 0) {
    return BST_get_data(root->left, key);
  }
  // more than 0 -> right son
  else { // key_comparison > 0
    return BST_get_data(root->right, key);
  }
}

int compare_keys(BSTKey searched_key, BSTKey node_key) {
  // Parameters must not be NULL
  assert(searched_key != NULL);
  assert(node_key != NULL);

  // Lexicographical comparison
  return strcmp(searched_key, node_key);
}

void update_height(BSTNode node) {

  // node may be NULL
  if (node == NULL)
    return;

  int left_height = 0;
  int right_height = 0;

  // Set left/right height if possible
  if (node->left != NULL) {
    left_height = node->left->height;
  }
  if (node->right != NULL) {
    right_height = node->right->height;
  }

  // Update height of node with larger value+1
  if (left_height > right_height) {
    node->height = left_height + 1;
  } else {
    node->height = right_height + 1;
  }
}

int get_height_diff(BSTNode node) {

  // node may be NULL
  if (node == NULL)
    return 0;

  int left_height = 0;
  int right_height = 0;

  // Set left/right height if possible
  if (node->left != NULL) {
    left_height = node->left->height;
  }
  if (node->right != NULL) {
    right_height = node->right->height;
  }

  // Return height difference
  return right_height - left_height;
}

BSTNode rebalance_tree(BSTNode crit_node) {

  // Get height difference
  int crit_diff = get_height_diff(crit_node);

  // Assert assumption
  if (crit_diff != 0) {
    assert(crit_node != NULL);
  }

  // Detect type of imbalance
  // Critical node left heavy
  if (crit_diff < -1) {

    int left_diff = get_height_diff(crit_node->left);

    // Left son left heavy or balanced = LL
    if (left_diff <= 0) {
      crit_node = node_right_rotation(crit_node, crit_node->left);
    }
    // Left son right heavy = LR
    else {
      BSTNode left_son = crit_node->left;
      left_son = node_left_rotation(left_son, left_son->right);
      crit_node = node_right_rotation(crit_node, left_son);
    }
  }
  // Critical node right heavy
  else if (crit_diff > 1) {

    int right_diff = get_height_diff(crit_node->right);

    // Right son right heavy or balanced = RR
    if (right_diff >= 0) {
      crit_node = node_left_rotation(crit_node, crit_node->right);
    }
    // Right son left heavy = LR
    else {
      BSTNode right_son = crit_node->right;
      right_son = node_right_rotation(right_son, right_son->left);
      crit_node = node_left_rotation(crit_node, right_son);
    }
  }

  // Return node that took place of critical node
  return crit_node;
}

BSTNode node_right_rotation(BSTNode parent, BSTNode left_son) {

  // Nodes must not be NULL
  assert(parent != NULL);
  assert(left_son != NULL);

  // Save node pointer
  BSTNode transferred_node = left_son->right;

  // Perform rotation
  left_son->right = parent;
  parent->left = transferred_node;

  // Left son becomes parent
  return left_son;
}

BSTNode node_left_rotation(BSTNode parent, BSTNode right_son) {

  // Nodes must not be NULL
  assert(parent != NULL);
  assert(right_son != NULL);

  // Save node pointer
  BSTNode transferred_node = right_son->left;

  // Perform rotation
  right_son->left = parent;
  parent->right = transferred_node;

  // Right son becomes parent
  return right_son;
}
