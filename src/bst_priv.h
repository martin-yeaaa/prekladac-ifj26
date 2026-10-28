// bst_priv.h
// Authors: Martin Vaško (vaskom00)

// Header file for private declarations only visible to bst.c

#include "bst.h"

// Nodes of a binary tree store 2 pointers to children nodes
struct bst_node {

  BSTKey key;
  BSTData data;
  int height;

  struct bst_node *left;
  struct bst_node *right;
};

/* Declarations of internal functions */

/**
 * Figures out if tree is empty or not.
 * @param root Pointer to node, must not be NULL.
 * @return `true` if tree is empty, otherwise `false`.
 */
static bool is_empty(BSTNode *root);

/**
 * Compares keys, used for consistent comparison order.
 * @param searched_key Key which is being searched. Must not be `NULL`.
 * @param node_key Key saved in a node. Must not be `NULL`.
 * @return 0 on exact match, otherwise a positive or negative value.
 */
static int compare_keys(BSTKey searched_key, BSTKey node_key);

/**
 * Attempts to find node with a matching `key`. If no such node exists,
 * returns the last node in which keys were compared.
 * @param node Pointer to node. It must not point to an empty tree.
 * @param key Key of searched node. Must not be `NULL`.
 * @return Pointer to closest node.
 */
static BSTNode *find_closest_node(BSTNode *node, BSTKey key);

/**
 * Creates a new node.
 * @param key Key of created node. Must not be `NULL`.
 * @param data Data of created node. Must not be `NULL`.
 * @return Pointer to newly created node. If creation fails, returns `NULL`.
 */
static BSTNode *create_node(BSTKey key, BSTData data);

// TODO functions for height balancing
