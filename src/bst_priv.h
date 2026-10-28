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
 * @param root BST node.
 * @param key Key of the inserted node, must not be `NULL`.
 * @param data Data of the inserted node.
 * @return `root`, or a new node.
 */
BSTNode insert_recursive(BSTNode root, BSTKey key, BSTData data);

/**
 * Creates a new node.
 * @param key Key of created node. Must not be `NULL`.
 * @param data Data of created node.
 * @return Pointer to newly created node. If creation fails, returns `NULL`.
 */
BSTNode create_node(BSTKey key, BSTData data);

/**
 * Goes through tree recursively, deletes a node.
 * Updates height, maintains height balance.
 * @param root BST node.
 * @param key Key of the inserted node, must not be `NULL`.
 * @return `root`, or a node that took place of the deleted one.
 */
BSTNode delete_recursive(BSTNode root, BSTKey key);

/**
 * Deletes a node.
 * @param node Node to delete. Must not be `NULL`.
 * @return `BSTNode` that took place of the deleted node, or `NULL`.
 */
BSTNode delete_node(BSTNode node);

/**
 * Recursively gets the right most son.
 * @param node Parent node. Must not be `NULL`.
 * @return Right most son node.
 */
BSTNode get_right_most_son(BSTNode node);

// TODO functions for height balancing
