// bst.c
// Authors: Martin Vaško (vaskom00)

// Implementation of height-balanced binary search tree

#include "bst.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void BST_init(TNode *root) {

  // Parameters must not be NULL
  assert(root != NULL);

  // Initializes all values to NULL
  root->key = NULL; // If the key _inside_ BST is NULL, it is empty!
  root->data = NULL;
  root->left = NULL;
  root->right = NULL;
}

bool BST_insert(TNode *root, TKey key, TData data) {

  // Parameters must not be NULL
  assert(root != NULL);
  assert(key != NULL);
  assert(data != NULL);

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
