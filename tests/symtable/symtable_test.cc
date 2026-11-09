// Example test
// Author: Martin Vaško (xvaskom00)

#include <gtest/gtest.h>

extern "C" {
#include "../../src/bst.h"

// only for testing, dont include this in modules, just use symtable.h
#include "../../src/symtable.c"
}

// These tests don't check memory leaks, test separately!!

TEST(symbol_tables, new_ST) {

  SymbTables st = ST_new();
  EXPECT_TRUE(st.list_first == NULL);
  EXPECT_TRUE(st.list_last == NULL);
  EXPECT_TRUE(st.stack_top == NULL);

}

TEST(symbol_tables, enter_scope) {

  SymbTables st = ST_new();
  EXPECT_TRUE(st.list_first == NULL);
  EXPECT_TRUE(st.list_last == NULL);
  EXPECT_TRUE(st.stack_top == NULL);
  
  int ok = ST_enter_scope(&st);
  // Check success
  ASSERT_TRUE(ok == 0);
  // Check scope pointers
  EXPECT_TRUE(st.list_first != NULL);
  EXPECT_TRUE(st.list_last != NULL);
  EXPECT_TRUE(st.stack_top != NULL);
  // All pointers are the same
  EXPECT_TRUE(st.list_first == st.list_last);
  EXPECT_TRUE(st.list_last == st.stack_top);

  // Check linked node
  EXPECT_TRUE(st.stack_top->next_node == NULL);
  EXPECT_TRUE(st.stack_top->prev_scope == NULL);
  // Table is empty, so this should be NULL, unless BST implementation changes
  EXPECT_TRUE(st.stack_top->table == NULL);
}

TEST(symbol_tables, enter_multiple_scopes) {

  SymbTables st = ST_new();

  int ok = ST_enter_scope(&st);
  // Check success
  ASSERT_TRUE(ok == 0);

  ok = ST_enter_scope(&st);
  // Check success
  ASSERT_TRUE(ok == 0);

  // NOT all pointers are the same
  EXPECT_TRUE(st.list_first != st.list_last);
  // Right now these should be the same
  EXPECT_TRUE(st.list_last == st.stack_top);

  // Check linking
  EXPECT_TRUE(st.stack_top->prev_scope == st.list_first);
  EXPECT_TRUE(st.list_last->prev_scope == st.list_first);
  EXPECT_TRUE(st.list_first->next_node == st.list_last);
  EXPECT_TRUE(st.stack_top->next_node == NULL);
  EXPECT_TRUE(st.list_last->next_node == NULL);
  EXPECT_TRUE(st.list_first->prev_scope == NULL);

  // Try one more
  ok = ST_enter_scope(&st);
  // Check success
  ASSERT_TRUE(ok == 0);

  // NOT all pointers are the same
  EXPECT_TRUE(st.list_first != st.list_last);
  // Right now these should be the same
  EXPECT_TRUE(st.list_last == st.stack_top);

  // Stack linking
  EXPECT_TRUE(st.list_first == st.stack_top->prev_scope->prev_scope);

  // List linking
  EXPECT_TRUE(st.list_first->next_node->next_node == st.list_last);
}


TEST(symbol_tables, exit_scope) {

  // New symbol table
  SymbTables st = ST_new();
  
  int ok = ST_enter_scope(&st);
  // Check success
  ASSERT_TRUE(ok == 0);

  ST_exit_scope(&st);
  // Stack should be empty, list should stay untouched
  EXPECT_TRUE(st.stack_top == NULL);
  EXPECT_TRUE(st.list_first != NULL);
  EXPECT_TRUE(st.list_first == st.list_last);
}

TEST(symbol_tables, enter_exit_scopes) {

  // New symbol table
  SymbTables st = ST_new();

  int ok = ST_enter_scope(&st);
  ASSERT_TRUE(ok == 0);
  ok = ST_enter_scope(&st);
  ASSERT_TRUE(ok == 0);
  ok = ST_enter_scope(&st);
  ASSERT_TRUE(ok == 0);

  // Check stack
  EXPECT_TRUE(st.stack_top->prev_scope->prev_scope == st.list_first);
  // Check list
  EXPECT_TRUE(st.list_first->next_node->next_node == st.list_last);

  // Exit one scope
  ST_exit_scope(&st);
  // Check stack
  EXPECT_TRUE(st.stack_top->prev_scope == st.list_first);

  // Enter scopes
  ok = ST_enter_scope(&st);
  ASSERT_TRUE(ok == 0);
  ok = ST_enter_scope(&st);
  ASSERT_TRUE(ok == 0);

  // Check stack
  EXPECT_TRUE(st.stack_top->prev_scope->prev_scope->prev_scope == st.list_first);
  // Check list
  EXPECT_TRUE(st.list_first->next_node->next_node->next_node->next_node == st.list_last);

  // Exit scopes, leave one on stack
  ST_exit_scope(&st);
  ST_exit_scope(&st);
  ST_exit_scope(&st);

  // Check stack
  EXPECT_TRUE(st.stack_top == st.list_first);
  // Check list
  EXPECT_TRUE(st.list_first->next_node->next_node->next_node->next_node == st.list_last);

  ST_exit_scope(&st);
  // Check stack
  EXPECT_TRUE(st.stack_top == NULL);
  // Check list
  EXPECT_TRUE(st.list_first->next_node->next_node->next_node->next_node == st.list_last);

  ST_enter_scope(&st);
  // Check stack
  EXPECT_TRUE(st.stack_top == st.list_last);
  // Check list
  EXPECT_TRUE(st.list_first->next_node->next_node->next_node->next_node->next_node == st.list_last);
}

TEST(symbol_tables, destroy_table){

  // New symbol table
  SymbTables st = ST_new();

  int ok = ST_enter_scope(&st);
  ASSERT_TRUE(ok == 0);
  ok = ST_enter_scope(&st);
  ASSERT_TRUE(ok == 0);
  ok = ST_enter_scope(&st);
  ASSERT_TRUE(ok == 0);

  ST_exit_scope(&st);

  // This only tests if the program runs or not
  // Run separate memory leak tests!
  ST_destroy(&st);
}
