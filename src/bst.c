// bst.c
// Authors: Martin Vaško (vaskom00)

// Implementation of height-balanced binary search tree

#include "bst.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Declarations of internal functions */

/**
 * Figures out if tree is empty or not.
 * @param root Pointer to node, must not be NULL.
 * @return `true` if tree is empty, otherwise `false`.
 */
static bool is_empty(TNode *root);

/**
 * Compares keys, used for consistent comparison order.
 * @param searched_key Key which is being searched. Must not be `NULL`.
 * @param node_key Key saved in a node. Must not be `NULL`.
 * @return 0 on exact match, otherwise a positive or negative value.
 */
static int compare_keys(TKey searched_key, TKey node_key);

/**
 * Attempts to find node with a matching `key`. If no such node exists,
 * returns the last node in which keys were compared.
 * @param node Pointer to node. It must not point to an empty tree.
 * @param key Key of searched node. Must not be `NULL`.
 * @return Pointer to closest node.
 */
static TNode *find_closest_node(TNode *node, TKey key);

/**
 * Creates a new node.
 * @param key Key of created node. Must not be `NULL`.
 * @param data Data of created node. Must not be `NULL`.
 * @return Pointer to newly created node. If creation fails, returns `NULL`.
 */
static TNode *create_node(TKey key, TData data);

// TODO functions for height balancing

/* IMPLEMENTATION */

void BST_init(TNode *root) {

  // Parameters must not be NULL
  assert(root != NULL);

  // Initializes all values to NULL
  root->key = NULL; // If the key is NULL, BST is empty!
  root->data = NULL;
  root->left = NULL;
  root->right = NULL;
}

bool BST_insert(TNode *root, TKey key, TData data) {

  // Parameters must not be NULL
  assert(root != NULL);
  assert(key != NULL);
  assert(data != NULL);

  // If root is an empty tree, we can write directly into it
  if (is_empty(root)) {
    root->key = key;
    root->data = data;

    return true;
  }

  // If tree isn't empty, we search the tree
  TNode *closest_node = find_closest_node(root, key);

  // When comparing keys, first parameter is the searched key, second is node
  // key
  int key_comparison = compare_keys(key, closest_node->key);

  // Exact match -> rewrite data
  if (key_comparison == 0) {
    closest_node->data = data;
  }
  // No match -> create new node
  else {
    TNode *new_node = create_node(key, data);

    // Check if creation succeeded
    if (new_node == NULL)
      return false;

    // less than 0 -> left son
    if (key_comparison < 0) {
      closest_node->left = new_node;
    }
    // more than 0 -> right son
    else { // key_comparison > 0
      closest_node->right = new_node;
    }
  }

  return true;
}

void BST_delete(TNode *root, TKey key) {

  // Parameters must not be NULL
  assert(root != NULL);
  assert(key != NULL);
}

TData BST_get_data(TNode *root, TKey key) {

  // Parameters must not be NULL
  assert(root != NULL);
  assert(key != NULL);

  return NULL;
}

static bool is_empty(TNode *root) {

  // root must not be NULL
  assert(root != NULL);

  // Here, a tree is considered empty when key of root is NULL
  if (root->key == NULL)
    return true;
  else
    return false;
}

static int compare_keys(TKey searched_key, TKey node_key) {
  // Parameters must not be NULL
  assert(searched_key != NULL);
  assert(node_key != NULL);

  // Lexicographical comparison
  return strcmp(searched_key, node_key);
}

static TNode *find_closest_node(TNode *node, TKey key) {

  // If *node points to a tree, it must not be an empty tree
  if (node != NULL)
    assert(is_empty(node) == false);
  // key must not be NULL
  assert(key != NULL);
}

static TNode *create_node(TKey key, TData data) {

  // Parameters must not be NULL
  assert(key != NULL);
  assert(data != NULL);
}