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
  // Check private data and height
  EXPECT_EQ(*((int*)tree->data), 4);
  EXPECT_EQ(tree->height, 3);

  EXPECT_EQ(*((int*)tree->left->data), 2);
  EXPECT_EQ(*((int*)tree->right->data), 6);
  EXPECT_EQ(tree->left->height, 2);
  EXPECT_EQ(tree->right->height, 2);

  EXPECT_EQ(*((int*)tree->left->left->data), 1);
  EXPECT_EQ(*((int*)tree->left->right->data), 3);
  EXPECT_EQ(*((int*)tree->right->left->data), 5);
  EXPECT_EQ(*((int*)tree->right->right->data), 7);
  EXPECT_EQ(tree->left->left->height, 1);
  EXPECT_EQ(tree->left->right->height, 1);
  EXPECT_EQ(tree->right->left->height, 1);
  EXPECT_EQ(tree->right->right->height, 1);
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


TEST(BST_basic, func_BST_get_data) {

  // New tree
  BSTNode tree = BST_new();
  // Insert data
  int data = 5;
  char key[] = "hello";
  EXPECT_TRUE(BST_insert(&tree, key, &data));
  // Insert more data
  int data2 = 52;
  char key2[] = "aaaa";
  EXPECT_TRUE(BST_insert(&tree, key2, &data2));

  // Check if it's really there
  ASSERT_FALSE(tree == NULL);
  // Test get_data()
  EXPECT_TRUE(BST_get_data(tree, key) == &data);
  EXPECT_TRUE(BST_get_data(tree, key2) == &data2);
  // Test if it works correctly after deleting nodes
  BST_delete(&tree, key);
  EXPECT_TRUE(BST_get_data(tree, key) == NULL);
  EXPECT_TRUE(BST_get_data(tree, key2) == &data2);
  BST_delete(&tree, key2);
  EXPECT_TRUE(BST_get_data(tree, key) == NULL);
  EXPECT_TRUE(BST_get_data(tree, key2) == NULL);
  // Tree should be empty
  EXPECT_TRUE(tree == NULL);
}


class BST_balanced_fill : public testing::Test {
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
  
  BST_balanced_fill() {
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

TEST_F(BST_balanced_fill, fixture_test) {

  // Check if it's really there
  ASSERT_FALSE(tree == NULL);
  
  // Check private data
  EXPECT_EQ(*((int*)tree->data), 4);

  EXPECT_EQ(*((int*)tree->left->data), 2);
  EXPECT_EQ(*((int*)tree->right->data), 6);

  EXPECT_EQ(*((int*)tree->left->left->data), 1);
  EXPECT_EQ(*((int*)tree->left->right->data), 3);
  EXPECT_EQ(*((int*)tree->right->left->data), 5);
  EXPECT_EQ(*((int*)tree->right->right->data), 7);

  // Check height
  EXPECT_EQ(tree->height, 3);
  EXPECT_EQ(tree->left->height, 2);
  EXPECT_EQ(tree->right->height, 2);
  EXPECT_EQ(tree->left->left->height, 1);
  EXPECT_EQ(tree->left->right->height, 1);
  EXPECT_EQ(tree->right->left->height, 1);
  EXPECT_EQ(tree->right->right->height, 1);
}

TEST_F(BST_balanced_fill, func_BST_delete_all) {
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

TEST_F(BST_balanced_fill, func_BST_delete_nonterminal_with_left_son) {

  // Insert leftmost son (key "0" < "a")
  char leftkey[] = "0";
  int leftdata = 10;
  BST_insert(&tree, leftkey, &leftdata);
  
  // Check correct insertion
  ASSERT_TRUE(tree->left->left->left->data == &leftdata);
  EXPECT_EQ(tree->height, 4); // Height check

  // Delete node above newly inserted one
  BST_delete(&tree, keya);

  // Node with key "a" should be gone
  EXPECT_TRUE(BST_get_data(tree, keya) == NULL);
  // It's place should be taken by newly inserted node
  EXPECT_TRUE(tree->left->left->data == &leftdata);
  // Height check
  EXPECT_EQ(tree->height, 3);
  EXPECT_EQ(tree->left->height, 2);
  EXPECT_EQ(tree->left->left->height, 1);
}

TEST_F(BST_balanced_fill, func_BST_delete_nonterminal_with_right_son) {

  // The exact same thing as func_BST_delete_nonterminal_with_left_son
  // But it's the right son now

  char rightkey[] = "z";
  int rightdata = 20;
  BST_insert(&tree, rightkey, &rightdata);

  // Check correct insertion
  ASSERT_TRUE(tree->right->right->right->data == &rightdata);
  EXPECT_EQ(tree->height, 4); // Height check

  // Delete node above newly inserted one
  BST_delete(&tree, keyg);

  // Node with key "g" should be gone
  EXPECT_TRUE(BST_get_data(tree, keyg) == NULL);
  // It's place should be taken by newly inserted node
  EXPECT_TRUE(tree->right->right->data == &rightdata);
  // Height check
  EXPECT_EQ(tree->height, 3);
  EXPECT_EQ(tree->right->height, 2);
  EXPECT_EQ(tree->right->right->height, 1);
}

TEST_F(BST_balanced_fill, func_BST_delete_nonterminal_with_both_sons) {

  // Delete node with two sons
  BST_delete(&tree, keyd);
  // Check deletion
  ASSERT_TRUE(BST_get_data(tree, keyd) == NULL);
  EXPECT_EQ(tree->height, 3); // Height check

  // Replacement is right-most son of left subtree
  // In this case it's the node with key "c"
  ASSERT_EQ(strcmp(tree->key, keyc), 0);

  // A few more deletions
  BST_delete(&tree, keyc);
  ASSERT_TRUE(BST_get_data(tree, keyc) == NULL);
  ASSERT_EQ(strcmp(tree->key, keyb), 0);

  BST_delete(&tree, keyf);
  ASSERT_TRUE(BST_get_data(tree, keyf) == NULL);
  ASSERT_EQ(strcmp(tree->right->key, keye), 0);
  // Height check
  EXPECT_EQ(tree->height, 3);
  EXPECT_EQ(tree->left->height, 1);
  EXPECT_EQ(tree->right->height, 2);
}

TEST_F(BST_balanced_fill, func_BST_delete_complex) {

  // Insert another carefully placed node
  int data_s = 67;
  char key_s[3] = "bm";
  BST_insert(&tree, key_s, &data_s);
  // Check insertion
  ASSERT_TRUE(tree->left->right->left->data == &data_s);
  EXPECT_EQ(tree->height, 4); // Height check

  // Delete node with two sons
  BST_delete(&tree, keyd);
  // Check deletion
  ASSERT_TRUE(BST_get_data(tree, keyd) == NULL);
  // Replacement is right-most son of left subtree
  // In this case it's the node with key "c"
  ASSERT_EQ(strcmp(tree->key, keyc), 0);

  // Node with key "c" should be "replaced" by node with key "bm"
  ASSERT_EQ(strcmp(tree->left->right->key, key_s), 0);
  ASSERT_TRUE(tree->left->right->left == NULL);
  // Height check
  EXPECT_EQ(tree->height, 3);
  EXPECT_EQ(tree->left->height, 2);
  EXPECT_EQ(tree->right->height, 2);
}


TEST(BST_unbalanced, insert_LL) {

  BSTNode tree = BST_new();
  int data[] = {1, 2, 3};
  char keyc[2] = "c";
  char keyd[2] = "d";
  char keye[2] = "e";

  // Inserting only on left side
  BST_insert(&tree, keye, &data[0]);
  BST_insert(&tree, keyd, &data[1]);
  // No rotation yet
  ASSERT_TRUE(tree->data == &data[0]);
  ASSERT_TRUE(tree->left->data == &data[1]);

  // This will cause LL imbalance
  BST_insert(&tree, keyc, &data[2]);
  // Check correct rotation
  EXPECT_TRUE(tree->data == &data[1]);
  EXPECT_TRUE(tree->left->data == &data[2]);
  EXPECT_TRUE(tree->right->data == &data[0]);
  // Check height
  EXPECT_TRUE(tree->height == 2);
  EXPECT_TRUE(tree->left->height == 1);
  EXPECT_TRUE(tree->right->height == 1);

  // Sanity check
  EXPECT_TRUE(tree->left->left == NULL);
  EXPECT_TRUE(tree->left->right == NULL);
  EXPECT_TRUE(tree->right->left == NULL);
  EXPECT_TRUE(tree->right->right == NULL);

  // Try more
  int data_more[] = {6, 7};
  char keya[] = "a";
  char keyb[] = "b";

  BST_insert(&tree, keyb, &data_more[0]);
  BST_insert(&tree, keya, &data_more[1]);

  EXPECT_TRUE(tree->left->data == &data_more[0]);       // b
  EXPECT_TRUE(tree->left->left->data == &data_more[1]); // a
  EXPECT_TRUE(tree->left->right->data == &data[2]);     // c
  // Height
  EXPECT_TRUE(tree->height == 3);
  EXPECT_TRUE(tree->right->height == 1);
  EXPECT_TRUE(tree->left->height == 2);
  EXPECT_TRUE(tree->left->left->height == 1);
  EXPECT_TRUE(tree->left->right->height == 1);

}

TEST(BST_unbalanced, insert_RR) {

  BSTNode tree = BST_new();
  int data[] = {1, 2, 3};
  char keya[2] = "a";
  char keyb[2] = "b";
  char keyc[2] = "c";

  // Inserting only on right side
  BST_insert(&tree, keya, &data[0]);
  BST_insert(&tree, keyb, &data[1]);
  // No rotation yet
  ASSERT_TRUE(tree->data == &data[0]);
  ASSERT_TRUE(tree->right->data == &data[1]);

  // This will cause RR imbalance
  BST_insert(&tree, keyc, &data[2]);
  // Check correct rotation
  EXPECT_TRUE(tree->data == &data[1]);
  EXPECT_TRUE(tree->left->data == &data[0]);
  EXPECT_TRUE(tree->right->data == &data[2]);
  // Check height
  EXPECT_TRUE(tree->height == 2);
  EXPECT_TRUE(tree->left->height == 1);
  EXPECT_TRUE(tree->right->height == 1);
  
  // Sanity check
  EXPECT_TRUE(tree->left->left == NULL);
  EXPECT_TRUE(tree->left->right == NULL);
  EXPECT_TRUE(tree->right->left == NULL);
  EXPECT_TRUE(tree->right->right == NULL);

    // Try more
  int data_more[] = {6, 7};
  char keyd[] = "d";
  char keye[] = "e";

  BST_insert(&tree, keyd, &data_more[0]);
  BST_insert(&tree, keye, &data_more[1]);

  EXPECT_TRUE(tree->right->data == &data_more[0]);        // d
  EXPECT_TRUE(tree->right->left->data == &data[2]);       // c
  EXPECT_TRUE(tree->right->right->data == &data_more[1]); // e
  // Height
  EXPECT_TRUE(tree->height == 3);
  EXPECT_TRUE(tree->left->height == 1);
  EXPECT_TRUE(tree->right->height == 2);
  EXPECT_TRUE(tree->right->left->height == 1);
  EXPECT_TRUE(tree->right->right->height == 1);
}


class BST_left_heavy_fill : public testing::Test {
protected:

  BSTNode tree;
  int data[5] = {1, 2, 3, 4, 5};
  char keye[2] = "e";
  char keyg[2] = "g";
  char keyi[2] = "i";
  char keyk[2] = "k";
  char keym[2] = "m";

  BST_left_heavy_fill() {
    // New tree
    tree = BST_new();
    // Insert data
    // We insert data in a way so there's no need for internal rebalancing
    // However we expect correct sorting
    BST_insert(&tree, keyk, &data[0]);

    BST_insert(&tree, keyg, &data[1]);
    BST_insert(&tree, keym, &data[2]);

    BST_insert(&tree, keye, &data[3]);
    BST_insert(&tree, keyi, &data[4]);

    /*  Scheme of created tree (with keys as nodes):

                K
               / \
              G   M
             / \
            E   I
                  
    */
  }
};

TEST_F(BST_left_heavy_fill, fixture_test) {

  // Check if it's really there
  ASSERT_FALSE(tree == NULL);
  
  // Check private data
  EXPECT_EQ(*((int*)tree->data), 1);

  EXPECT_EQ(*((int*)tree->left->data), 2);
  EXPECT_EQ(*((int*)tree->right->data), 3);

  EXPECT_EQ(*((int*)tree->left->left->data), 4);
  EXPECT_EQ(*((int*)tree->left->right->data), 5);

  // Check height
  EXPECT_EQ(tree->height, 3);
  EXPECT_EQ(tree->left->height, 2);
  EXPECT_EQ(tree->right->height, 1);
  EXPECT_EQ(tree->left->left->height, 1);
  EXPECT_EQ(tree->left->right->height, 1);
}

TEST_F(BST_left_heavy_fill, insert_LR_1) {

  int lrdata = 67;
  char keyj[2] = "j";

  // This will cause LR rotation
  BST_insert(&tree, keyj, &lrdata);

  // Check each node
  EXPECT_TRUE(strcmp(tree->key, "i") == 0);
  EXPECT_TRUE(strcmp(tree->right->key, "k") == 0);
  EXPECT_TRUE(strcmp(tree->right->right->key, "m") == 0);
  EXPECT_TRUE(strcmp(tree->right->left->key, "j") == 0);
  EXPECT_TRUE(strcmp(tree->left->key, "g") == 0);
  EXPECT_TRUE(strcmp(tree->left->left->key, "e") == 0);

  // Height check
  EXPECT_TRUE(tree->height == 3);
  EXPECT_TRUE(tree->right->height == 2);
  EXPECT_TRUE(tree->right->right->height == 1);
  EXPECT_TRUE(tree->right->left->height == 1);
  EXPECT_TRUE(tree->left->height == 2);
  EXPECT_TRUE(tree->left->left->height == 1);
}

TEST_F(BST_left_heavy_fill, insert_LR_2) {

  int lrdata = 67;
  char keyj[2] = "h";

  // This will cause LR rotation
  BST_insert(&tree, keyj, &lrdata);

  // Check each node
  EXPECT_TRUE(strcmp(tree->key, "i") == 0);
  EXPECT_TRUE(strcmp(tree->right->key, "k") == 0);
  EXPECT_TRUE(strcmp(tree->right->right->key, "m") == 0);
  EXPECT_TRUE(strcmp(tree->left->right->key, "h") == 0);
  EXPECT_TRUE(strcmp(tree->left->key, "g") == 0);
  EXPECT_TRUE(strcmp(tree->left->left->key, "e") == 0);

  // Height check
  EXPECT_TRUE(tree->height == 3);
  EXPECT_TRUE(tree->right->height == 2);
  EXPECT_TRUE(tree->right->right->height == 1);
  EXPECT_TRUE(tree->left->right->height == 1);
  EXPECT_TRUE(tree->left->height == 2);
  EXPECT_TRUE(tree->left->left->height == 1);
}



class BST_right_heavy_fill : public testing::Test {
protected:

  BSTNode tree;
  int data[5] = {1, 2, 3, 4, 5};
  char keye[2] = "e";
  char keyg[2] = "g";
  char keyi[2] = "i";
  char keyk[2] = "k";
  char keym[2] = "m";

  BST_right_heavy_fill() {
    // New tree
    tree = BST_new();
    // Insert data
    // We insert data in a way so there's no need for internal rebalancing
    // However we expect correct sorting
    BST_insert(&tree, keyg, &data[1]);

    BST_insert(&tree, keye, &data[3]);
    BST_insert(&tree, keyk, &data[0]);
    
    BST_insert(&tree, keym, &data[2]);
    BST_insert(&tree, keyi, &data[4]);

    /*  Scheme of created tree (with keys as nodes):

                G
               / \
              E   K
                 / \
                I   M
                  
    */
  }
};

TEST_F(BST_right_heavy_fill, fixture_test) {

  // Check if it's really there
  ASSERT_FALSE(tree == NULL);
  
  // Check private data
  EXPECT_EQ(*((int*)tree->data), 2);

  EXPECT_EQ(*((int*)tree->left->data), 4);
  EXPECT_EQ(*((int*)tree->right->data), 1);

  EXPECT_EQ(*((int*)tree->right->left->data), 5);
  EXPECT_EQ(*((int*)tree->right->right->data), 3);

  // Check height
  EXPECT_EQ(tree->height, 3);
  EXPECT_EQ(tree->left->height, 1);
  EXPECT_EQ(tree->right->height, 2);
  EXPECT_EQ(tree->right->left->height, 1);
  EXPECT_EQ(tree->right->right->height, 1);
}