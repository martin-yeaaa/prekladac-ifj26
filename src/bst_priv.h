// Project: prekladac-ifj26
// File: bst_priv.h
// Author: Martin Vaško (xvaskom00)

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
 * @param data_size sizeof(data)
 * @return `root`, or a new node.
 */
BSTNode insert_recursive(BSTNode root, BSTKey key, BSTData data,
                         size_t data_size);

/**
 * Creates a new node.
 * @param key Key of created node. Must not be `NULL`.
 * @param data Data of created node.
 * @param data_size sizeof(data)
 * @return Pointer to newly created node. If creation fails, returns `NULL`.
 */
BSTNode create_node(BSTKey key, BSTData data, size_t data_size);

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

/**
 * Sets height of a node based on its subtrees.
 * @param node Node which we want to update.
 */
void update_height(BSTNode node);

/**
 * Gets height difference of subtrees.
 * @param node Parent node.
 * @return Difference in height of subtrees.
 * Negative value means left subtree is taller.
 * Positive value means right subtree is taller.
 * Zero means tree is balanced
 */
int get_height_diff(BSTNode node);

/**
 * Detects type of imbalance (LL, LR, RL, RR) and performs rotations to make the
 * tree height-balanced. If tree is balanced, nothing happens.
 * @param crit_node Critical node in imbalanced tree.
 * @return Node that takes place of critical node.
 */
BSTNode rebalance_tree(BSTNode crit_node);

/**
 * Performs right rotation of tree nodes.
 * @param parent Parent node. Must not be `NULL`.
 * @param left_son Left son of parent node. Must not be `NULL`.
 * @return Node that takes place of parent node.
 */
BSTNode node_right_rotation(BSTNode parent, BSTNode left_son);

/**
 * Performs left rotation of tree nodes.
 * @param parent Parent node. Must not be `NULL`.
 * @param right_son Right son of parent node. Must not be `NULL`.
 * @return Node that takes place of parent node.
 */
BSTNode node_left_rotation(BSTNode parent, BSTNode right_son);
