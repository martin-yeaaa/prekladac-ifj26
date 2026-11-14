// Project: prekladac-ifj26
// File: test.c
// Author: Martin Vaško (xvaskom00)

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
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

BSTNode fill_tree(BSTNode tree) {

    // Make arrays of data to put in:
    // Tree makes its own copies of keys, you can free this from memory
    char *keys[ARRAY_LEN] = {"id", "int", "double", "str", "bool"};

    // Tree makes its own copies of data, you can free this too :DDD
    example_enum_t data[ARRAY_LEN] = {IDENTIFIER, INT, DOUBLE, STRING, BOOL};

    for (int i = 0; i < ARRAY_LEN; i++) {

        // Notice the & before &tree and &data
        // We must check if insertion succeeded! during insertion, malloc may fail!
        bool success = BST_insert(&tree, keys[i], &data[i], sizeof(data));
        if (success == false) {
            exit(1);
        }
    }

    // After we return from function, arrays of keys and data will be gone from memory, but its ok because tree makes its own copies
    return tree;
}

int main()
{
    // Filling tree example

    // Create new tree
    BSTNode tree = BST_new();

    // You can fill the tree in a separate funcion
    tree = fill_tree(tree);

    // BST_get_data returns pointer if data is found, or NULL otherwise
    example_enum_t *val = BST_get_data(tree, "double");

    printf("first case: ");
    // Check if NULL
    if (val != NULL) {
        printf("found data!! %d\n", *val);
    } else {
        printf("nothing :(");
    }
    
    // Rewrite test
    char new_data[] = "how??";
    bool success = BST_insert(&tree, "double", new_data, sizeof(new_data));
    if (success == false) exit(1);

    char *val_too = BST_get_data(tree, "double");
    printf("second case: ");
    // Check if NULL
    if (val_too != NULL) {
        printf("found data!! %s\n", val_too);
    } else {
        printf("nothing :(");
    }

    // Now we delete that node
    BST_delete(&tree, "double");

    // Update val
    val = BST_get_data(tree, "double");

    printf("third case: ");
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