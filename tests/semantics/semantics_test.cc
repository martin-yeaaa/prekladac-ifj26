// Semantics test
// Author: Martin Vaško

#include <gtest/gtest.h>


extern "C" {
#include "../../src/semantics.h"
#include <string.h>
}


TEST(build_AST, node_init_w_symbol) {

  ASTNode tree;

  // init node equivalent to: "x"
  int err = AST_init_node(&tree, NODE_IDENTIFIER, DT_INTEGER, "x");
  ASSERT_TRUE(err == 0);

  // Check correct initialization
  EXPECT_TRUE(tree->node_type == NODE_IDENTIFIER);
  EXPECT_TRUE(tree->data_type == DT_INTEGER);
  EXPECT_TRUE(strcmp(tree->data.string_val, "x") == 0);
  EXPECT_TRUE(tree->symbol_ptr == NULL);
  EXPECT_TRUE(tree->child_arr == NULL);
  EXPECT_TRUE(tree->child_count == 0);
  EXPECT_TRUE(tree->child_limit == 0);
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
  EXPECT_TRUE(tree->child_arr == NULL);
  EXPECT_TRUE(tree->child_count == 0);
  EXPECT_TRUE(tree->child_limit == 0);
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
  EXPECT_TRUE(tree->child_arr == NULL);
  EXPECT_TRUE(tree->child_count == 0);
  EXPECT_TRUE(tree->child_limit == 0);
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
  EXPECT_TRUE(tree->child_arr == NULL);
  EXPECT_TRUE(tree->child_count == 0);
  EXPECT_TRUE(tree->child_limit == 0);

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
  EXPECT_TRUE(second->child_arr == NULL);
  EXPECT_TRUE(second->child_count == 0);
  EXPECT_TRUE(tree->child_limit == 0);
}


TEST(build_AST, make_tree_var_def) {

  ASTNode definition;
  ASTNode variable;
  ASTNode integer;

  // convert into AST: var xd: Int = 5

  int err = AST_init_node(&definition, NODE_VAR_DEF, DT_INTEGER, NULL);
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&variable, NODE_IDENTIFIER, DT_INTEGER, "xd");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&integer, NODE_LITERAL, DT_INTEGER, "5");
  ASSERT_TRUE(err == 0);

  // Make tree by adding children separately
  err = AST_node_add_child(definition, variable);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(definition, integer);
  ASSERT_TRUE(err == 0);

  // Check tree
  EXPECT_TRUE(definition->child_arr[0]->node_type == NODE_IDENTIFIER);
  EXPECT_TRUE(definition->child_arr[1]->node_type == NODE_LITERAL);
}

TEST(build_AST, make_tree_fun_call) {

  ASTNode func_id;
  ASTNode func_call;
  ASTNode arg_list;
  ASTNode arg1;
  ASTNode arg2;
  ASTNode arg3;
  ASTNode arg4;
  ASTNode arg5;


  // convert into AST: func_id("too", "plus", 2, "equals", 4)

  // Init all nodes
  int err = AST_init_node(&func_id, NODE_IDENTIFIER, DT_INTEGER, "foo");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&func_call, NODE_FUNC_CALL, DT_UNDEF, NULL);
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&arg_list, NODE_ARG_LIST, DT_UNDEF, NULL);
  ASSERT_TRUE(err == 0);

  err = AST_init_node(&arg1, NODE_LITERAL, DT_STRING, "too");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&arg2, NODE_LITERAL, DT_STRING, "plus");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&arg3, NODE_LITERAL, DT_INTEGER, "2");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&arg4, NODE_LITERAL, DT_STRING, "equals");
  ASSERT_TRUE(err == 0);
  err = AST_init_node(&arg5, NODE_LITERAL, DT_INTEGER, "4");
  ASSERT_TRUE(err == 0);

  // Make tree by adding children separately
  err = AST_node_add_child(func_call, func_id);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(func_call, arg_list);
  ASSERT_TRUE(err == 0);

  err = AST_node_add_child(arg_list, arg1);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(arg_list, arg2);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(arg_list, arg3);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(arg_list, arg4);
  ASSERT_TRUE(err == 0);
  err = AST_node_add_child(arg_list, arg5);
  ASSERT_TRUE(err == 0);


  // Check tree
  EXPECT_TRUE(func_call->node_type == NODE_FUNC_CALL);
  EXPECT_TRUE(func_call->child_arr[0]->node_type == NODE_IDENTIFIER);
  EXPECT_TRUE(func_call->child_arr[1]->node_type == NODE_ARG_LIST);

  EXPECT_TRUE(arg_list->child_arr[0]->node_type == NODE_LITERAL);
  EXPECT_TRUE(arg_list->child_arr[1]->node_type == NODE_LITERAL);
  EXPECT_TRUE(arg_list->child_arr[2]->node_type == NODE_LITERAL);
  EXPECT_TRUE(arg_list->child_arr[3]->node_type == NODE_LITERAL);
  EXPECT_TRUE(arg_list->child_arr[4]->node_type == NODE_LITERAL);

}
