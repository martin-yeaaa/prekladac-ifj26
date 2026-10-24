// bst.h
// Authors: Martin Vaško (vaskom00)

// Header file for height-balanced binary search tree implementation

// The tree uses strings as keys
typedef char *TKey;

// For our purposes, the tree needs to store pointers to any kind of data
typedef void *TData;

// Nodes of a binary tree store 2 pointers to children nodes
typedef struct tree_node {
  TKey key;
  TData data;
  struct tree_node *left;
  struct tree_node *right;
} TNode;
typedef TNode *TNodePtr; // Helper typedef

/**
 * Initializes declared binary search tree data structure.
 * @param root Pointer to uninitialized BST root.
 */
void BST_init(TNode *root);

/**
 * Inserts a new node into the tree, balances tree height if needed.
 * If a node with the same `key` already exists, the data gets overwritten.
 * @param root Pointer to BST root.
 * @param key Key of the inserted node.
 * @param data Data of the inserted node.
 */
void BST_insert(TNode *root, TKey key, TData data);

/**
 * Deletes a node with matching `key`, balances tree height if needed.
 * If a node with the same `key` doesn't exist, nothing happens.
 * @param root Pointer to BST root.
 * @param key Key of the inserted node.
 */
void BST_delete(TNode *root, TKey key);

/**
 * Searches the tree with `key` for data.
 * @return `TData` if key is found, otherwise returns `NULL`
 * @param root Pointer to BST root.
 * @param key Key of the searched node.
 */
TData BST_get_data(TNode *root, TKey key);
