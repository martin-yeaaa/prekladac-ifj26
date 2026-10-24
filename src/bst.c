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
 * Figures out if tree is empty or not
 * @param root Pointer to node, must not be NULL.
 * @return `true` if tree is empty, otherwise `false`.
 */
static bool is_empty(TNode *root);
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