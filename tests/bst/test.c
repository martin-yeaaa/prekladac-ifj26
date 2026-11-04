// Project: prekladac-ifj26
// File: test.c
// Author: Martin Vaško (xvaskom00)

#include <stdio.h>
#include <string.h>
#include "../../src/bst.h"

typedef enum example_enum {
  IDENTIFIER,
  INT,
  DOUBLE,
  STRING,
  BOOL
} example_enum_t;

#define ARRAY_LEN 5

int main()
{
    // Filling tree example

    // Create new tree
    BSTNode tree = BST_new();

    // Make arrays of data to put in:

    // Tree makes its own copies of keys, you can free this from memory
    char *keys[ARRAY_LEN] = {"id", "int", "double", "str", "bool"};

    // Tree does NOT make its own copies of data, it only keeps pointers!!
    example_enum_t data[ARRAY_LEN] = {IDENTIFIER, INT, DOUBLE, STRING, BOOL};

    for (int i = 0; i < ARRAY_LEN; i++) {

        // Notice the & before &tree and &data
        BST_insert(&tree, keys[i], &data[i]);
    }

    // BST_get_data returns pointer if data is found, or NULL otherwise
    example_enum_t *val = BST_get_data(tree, keys[2]);

    printf("first case: ");
    // Check if NULL
    if (val != NULL) {
        printf("found data!! %d\n", *val);
    } else {
        printf("nothing :(");
    }
    
    // Now we delete that node
    BST_delete(&tree, keys[2]);

    // Update val
    val = BST_get_data(tree, keys[2]);

    printf("second case: ");
    // Check if NULL
    if (val != NULL) {
        printf("found data!! %d\n", *val);
    } else {
        printf("nothing :(\n");
    }
    
    // After we are done using the tree, remove it from memory
    BST_destroy_tree(tree);

    return 0;
}