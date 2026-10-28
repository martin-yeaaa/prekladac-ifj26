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
 * Compares keys, used for consistent comparison order.
 * @param searched_key Key which is being searched. Must not be `NULL`.
 * @param node_key Key saved in a node. Must not be `NULL`.
 * @return 0 on exact match, otherwise a positive or negative value.
 */
int compare_keys(BSTKey searched_key, BSTKey node_key);

/**
 * Goes through tree recursively, creates a new node or rewrites data of
 * an existing node. Updates height, maintains height balance.
 * In case of an error sets `error_flag = true`.
 * @param root BST root.
 * @param key Key of the inserted node, must not be `NULL`.
 * @param data Data of the inserted node.
 * @return Value of type `BSTNode`.
 */
BSTNode insert_recursive(BSTNode root, BSTKey key, BSTData data);

/**
 * Attempts to find node with a matching `key`. If no such node exists,
 * returns the last node in which keys were compared.
 * @param node Pointer to node. It must not point to an empty tree.
 * @param key Key of searched node. Must not be `NULL`.
 * @return Pointer to closest node.
 */
BSTNode find_closest_node(BSTNode node, BSTKey key);

/**
 * Creates a new node.
 * @param key Key of created node. Must not be `NULL`.
 * @param data Data of created node. Must not be `NULL`.
 * @return Pointer to newly created node. If creation fails, returns `NULL`.
 */
BSTNode create_node(BSTKey key, BSTData data);

// TODO functions for height balancing
