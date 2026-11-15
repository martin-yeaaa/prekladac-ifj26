// Semantics test
// Author: Martin Vaško

#include <gtest/gtest.h>


extern "C" {
#include "../../src/semantics.h"
#include <string.h>
}


TEST(build_AST, node_init_w_symbol) {

  ASTNode tree;

  // init node equivalent to: "var x: Int"
  int err = AST_init_node(&tree, NODE_VAR_DEF, DT_INTEGER, "x");
  ASSERT_TRUE(err == 0);

  // Check correct initialization
  EXPECT_TRUE(tree->node_type == NODE_VAR_DEF);
  EXPECT_TRUE(tree->data_type == DT_INTEGER);
  EXPECT_TRUE(strcmp(tree->data.string_val, "x") == 0);
  EXPECT_TRUE(tree->symbol_ptr == NULL);
  EXPECT_TRUE(tree->child_node_arr == NULL);
  EXPECT_TRUE(tree->child_node_count == 0);
}


TEST(build_AST, node_init_w_int_literal) {

  ASTNode tree;

  // init node equivalent to: "67"
  int err = AST_init_node(&tree, NODE_LITERAL, DT_INTEGER, "67");
  ASSERT_TRUE(err == 0);

  // Check correct initialization
  EXPECT_TRUE(tree->node_type == NODE_LITERAL);
  EXPECT_TRUE(tree->data_type == DT_INTEGER);
  EXPECT_TRUE(tree->data.int_val == 67);
  EXPECT_TRUE(tree->symbol_ptr == NULL);
  EXPECT_TRUE(tree->child_node_arr == NULL);
  EXPECT_TRUE(tree->child_node_count == 0);
}

TEST(build_AST, node_init_w_int_literal_invalid) {

  ASTNode tree;

  // init node equivalent to: "2147483648"
  int err = AST_init_node(&tree, NODE_LITERAL, DT_INTEGER, "2147483648");
  // Integer is out of range
  ASSERT_TRUE(err == 6);

  // "A"
  err = AST_init_node(&tree, NODE_LITERAL, DT_INTEGER, "A");
  // Invalid value
  ASSERT_TRUE(err == 6);
  
  // String cannot be "", that is a programmming error, checked by assert()

  // init node equivalent to: "2147483647"
  err = AST_init_node(&tree, NODE_LITERAL, DT_INTEGER, "2147483647");
  // Integer is not out of range
  ASSERT_TRUE(err == 0);
}

TEST(build_AST, node_init_w_double_literal) {

  ASTNode tree;

  // init node equivalent to: "123.456"
  int err = AST_init_node(&tree, NODE_LITERAL, DT_DOUBLE, "123.456");
  ASSERT_TRUE(err == 0);

  // Check correct initialization
  EXPECT_TRUE(tree->node_type == NODE_LITERAL);
  EXPECT_TRUE(tree->data_type == DT_DOUBLE);

  EXPECT_DOUBLE_EQ(tree->data.double_val, 123.456);

  EXPECT_TRUE(tree->symbol_ptr == NULL);
  EXPECT_TRUE(tree->child_node_arr == NULL);
  EXPECT_TRUE(tree->child_node_count == 0);
}

TEST(build_AST, node_init_w_double_literal_invalid) {

  ASTNode tree;

  // init node equivalent to: too large number
  int err = AST_init_node(&tree, NODE_LITERAL, DT_DOUBLE, "1211111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111113.456");
  ASSERT_TRUE(err == 6);

  // invalid value
  err = AST_init_node(&tree, NODE_LITERAL, DT_DOUBLE, "3.2 - that is three point two");
  ASSERT_TRUE(err == 6);
}

TEST(build_AST, node_init_w_string_literal) {

  ASTNode tree;

  // init node equivalent to: ""what how""
  int err = AST_init_node(&tree, NODE_LITERAL, DT_STRING, "what how");
  ASSERT_TRUE(err == 0);

  // Check correct initialization
  EXPECT_TRUE(tree->node_type == NODE_LITERAL);
  EXPECT_TRUE(tree->data_type == DT_STRING);

  EXPECT_TRUE(strcmp(tree->data.string_val, "what how") == 0);

  EXPECT_TRUE(tree->symbol_ptr == NULL);
  EXPECT_TRUE(tree->child_node_arr == NULL);
  EXPECT_TRUE(tree->child_node_count == 0);

  // One more node
  ASTNode second;
  
  // This can be empty string
  err = AST_init_node(&second, NODE_LITERAL, DT_STRING, "");
  ASSERT_TRUE(err == 0);

  // Check correct initialization
  EXPECT_TRUE(second->node_type == NODE_LITERAL);
  EXPECT_TRUE(second->data_type == DT_STRING);

  EXPECT_TRUE(strcmp(second->data.string_val, "") == 0);

  EXPECT_TRUE(second->symbol_ptr == NULL);
  EXPECT_TRUE(second->child_node_arr == NULL);
  EXPECT_TRUE(second->child_node_count == 0);

  
}