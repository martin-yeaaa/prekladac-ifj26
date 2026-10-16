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
typedef TNode *TNodePtr;
