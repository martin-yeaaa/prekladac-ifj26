// Project: prekladac-ifj26
// File: bst.h
// Author: Martin Vaško (xvaskom00)

// Header file for height-balanced binary search tree implementation

#include <stdbool.h>
#include <stddef.h>

// The tree uses strings as keys
typedef char *BSTKey;

// For our purposes, the tree needs to store pointers to any kind of data
typedef void *BSTData;

// Declared structure for tree nodes
typedef struct bst_node *BSTNode; // BSTNode is a pointer to struct

/**
 * Creates new binary search tree data structure.
 * @return New BST root node.
 */
BSTNode BST_new();

/**
 * Inserts a new node into the tree, balances tree height if needed.
 * If a node with the same `key` already exists, the data gets overwritten.
 * @param root Pointer(!) to BST root.
 * @param key Key of the inserted node, must not be `NULL`.
 * @param data Pointer to data of the inserted node.
 * @param data_size sizeof(data)
 *
 * @return `true` if insertion is succesful.
 * @return If memory allocation fails, writes an error to `stderr` and returns
 * `false`.
 */
bool BST_insert(BSTNode *root, BSTKey key, BSTData data, size_t data_size);

/**
 * Deletes a node with matching `key`, balances tree height if needed.
 * If a node with the same `key` doesn't exist, nothing happens.
 * @param root Pointer(!) to BST root.
 * @param key Key of the inserted node, must not be `NULL`.
 */
void BST_delete(BSTNode *root, BSTKey key);

/**
 * Searches the tree for data with associated `key`.
 * @param root BST root.
 * @param key Key of the searched node, must not be `NULL`.
 *
 * @return `BSTData` if key is found. Otherwise returns `NULL`.
 */
BSTData BST_get_data(BSTNode root, BSTKey key);

/**
 * Deletes the entire tree, frees it from memory.
 * @param root BST root. After performing this, `root` becomes unusable!
 */
void BST_destroy_tree(BSTNode root);