// Binary search tree test
// Author: Martin Vaško

#include <gtest/gtest.h>

extern "C" {
#include "../../src/bst.h"

 // For testing only; don't include bst_priv.h when using the module correctly!
#include "../../src/bst_priv.h"
}



TEST(BST_basic, func_BST_new) {

  BSTNode tree = BST_new();
  EXPECT_TRUE(tree == NULL);
  
}

TEST(BST_basic, func_BST_insert_empty) {

  // New tree
  BSTNode tree = BST_new();
  // Insert data
  int data = 5;
  char key[] = "hello";
  EXPECT_TRUE(BST_insert(&tree, key, &data));

  // Check if it's really there
  ASSERT_FALSE(tree == NULL);
  // Check private data
  EXPECT_EQ(tree->height, 1);
  EXPECT_EQ(tree->data, &data);
}

TEST(BST_basic, func_BST_insert_many) {

  // New tree
  BSTNode tree = BST_new();
  // Insert data
  int data[] = {1, 2, 3, 4, 5, 6, 7};
  char keya[] = "a";
  char keyb[] = "b";
  char keyc[] = "c";
  char keyd[] = "d";
  char keye[] = "e";
  char keyf[] = "f";
  char keyg[] = "g";

  // We insert data in a way so there's no need for internal rebalancing
  // However we expect correct sorting
  EXPECT_TRUE(BST_insert(&tree, keyd, &data[3]));

  EXPECT_TRUE(BST_insert(&tree, keyb, &data[1]));
  EXPECT_TRUE(BST_insert(&tree, keyf, &data[5]));

  EXPECT_TRUE(BST_insert(&tree, keya, &data[0]));
  EXPECT_TRUE(BST_insert(&tree, keyc, &data[2]));
  EXPECT_TRUE(BST_insert(&tree, keye, &data[4]));
  EXPECT_TRUE(BST_insert(&tree, keyg, &data[6]));

  // Check if it's really there
  ASSERT_FALSE(tree == NULL);
  // Check private data
  // TODO HEIGHT TEST WHEN HEIGHT UPDATE IS IMPLEMENTED
  EXPECT_EQ(*((int*)tree->data), 4);

  EXPECT_EQ(*((int*)tree->left->data), 2);
  EXPECT_EQ(*((int*)tree->right->data), 6);

  EXPECT_EQ(*((int*)tree->left->left->data), 1);
  EXPECT_EQ(*((int*)tree->left->right->data), 3);
  EXPECT_EQ(*((int*)tree->right->left->data), 5);
  EXPECT_EQ(*((int*)tree->right->right->data), 7);
}


TEST(BST_basic, func_BST_delete) {
  // New tree
  BSTNode tree = BST_new();
  // Insert data
  int data = 5;
  char key[] = "hello";
  EXPECT_TRUE(BST_insert(&tree, key, &data));

  // Check if it's really there
  ASSERT_FALSE(tree == NULL);
  // Check private data
  EXPECT_EQ(tree->height, 1);
  EXPECT_EQ(tree->data, &data);

  // Delete data
  BST_delete(&tree, key);
  // Tree should be empty
  EXPECT_TRUE(tree == NULL);
}

class BST_friendly_fill : public testing::Test {
protected:

  BSTNode tree;
  int data[7] = {1, 2, 3, 4, 5, 6, 7};
  char keya[2] = "a";
  char keyb[2] = "b";
  char keyc[2] = "c";
  char keyd[2] = "d";
  char keye[2] = "e";
  char keyf[2] = "f";
  char keyg[2] = "g";
  
  BST_friendly_fill() {
    // New tree
    tree = BST_new();
    // Insert data
    // We insert data in a way so there's no need for internal rebalancing
    // However we expect correct sorting
    BST_insert(&tree, keyd, &data[3]);

    BST_insert(&tree, keyb, &data[1]);
    BST_insert(&tree, keyf, &data[5]);

    BST_insert(&tree, keya, &data[0]);
    BST_insert(&tree, keyc, &data[2]);
    BST_insert(&tree, keye, &data[4]);
    BST_insert(&tree, keyg, &data[6]);

    /*  Scheme of created tree (with keys as nodes):

                 D
               /    \
              B      F
             / \    / \
            A   C  E   G

    */
  }
};

TEST_F(BST_friendly_fill, fixture_test) {

  // Check if it's really there
  ASSERT_FALSE(tree == NULL);
  
  // Check private data
  // TODO HEIGHT TEST WHEN HEIGHT UPDATE IS IMPLEMENTED
  EXPECT_EQ(*((int*)tree->data), 4);

  EXPECT_EQ(*((int*)tree->left->data), 2);
  EXPECT_EQ(*((int*)tree->right->data), 6);

  EXPECT_EQ(*((int*)tree->left->left->data), 1);
  EXPECT_EQ(*((int*)tree->left->right->data), 3);
  EXPECT_EQ(*((int*)tree->right->left->data), 5);
  EXPECT_EQ(*((int*)tree->right->right->data), 7);
}

TEST_F(BST_friendly_fill, func_BST_delete_all) {
  // Delete every entry
  BST_delete(&tree, keya);
  BST_delete(&tree, keyb);
  BST_delete(&tree, keyc);
  BST_delete(&tree, keyd);
  BST_delete(&tree, keye);
  BST_delete(&tree, keyf);
  BST_delete(&tree, keyg);

  // Tree should be empty
  EXPECT_TRUE(tree == NULL);
}

TEST_F(BST_friendly_fill, func_BST_delete_nonterminal_with_left_son) {

  // Insert leftmost son (key "0" < "a")
  char leftkey[] = "0";
  int leftdata = 10;
  BST_insert(&tree, leftkey, &leftdata);

  // Check correct insertion
  ASSERT_TRUE(tree->left->left->left->data == &leftdata);

  // Delete node above newly inserted one
  BST_delete(&tree, keya);

  // Node with key "a" should be gone
  EXPECT_TRUE(BST_get_data(tree, keya) == NULL);
  // It's place should be taken by newly inserted node
  EXPECT_TRUE(tree->left->left->data == &leftdata);
} 