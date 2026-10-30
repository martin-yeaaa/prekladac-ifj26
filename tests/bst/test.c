// Project: prekladac-ifj26
// File: test.c
// Author: Martin Vaško (xvaskom00)

#include <stdio.h>
#include <string.h>
#include "../../src/bst.h"

int main()
{
    // Small usage sample
    BSTNode tree = BST_new();

    char key[] = "hi";
    int data = 1337;

    BST_insert(&tree, key, &data);

    printf("i am %d\n", *(int*)BST_get_data(tree, key));

    BST_delete(&tree, key);

    return 0;
}