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


class symbol_definitions : public testing::Test {
protected:

  SymbTables st;
  SymbDraft x_val_int;
  SymbDraft x_var_double;
  SymbDraft y_var_string;
  SymbDraft z_var_bool;
  SymbDraft smol_val_unit; // could also be var

  SymbDraft foo_func; // returns double, has 2 parameters w types int, double
  DataType param_arr[2] = {DT_INTEGER, DT_DOUBLE};

  SymbDraft foo_main_func;
  SymbDraft big_list_symbol; // list of 11 ddoouubblleess

  symbol_definitions() {

    st = ST_new();
    
    // Initialize symbol drafts
    x_val_int = {
      .identifier = "x",
      .symb_type = SB_VAL,
      .data_type = DT_INTEGER,
    };

    x_var_double = {
      .identifier = "x",
      .symb_type = SB_VAR,
      .data_type = DT_DOUBLE,
    };

    y_var_string = {
      .identifier = "y",
      .symb_type = SB_VAR,
      .data_type = DT_DOUBLE,
    };

    z_var_bool = {
      .identifier = "z",
      .symb_type = SB_VAR,
      .data_type = DT_BOOL,
    };

    smol_val_unit = {
      .identifier = "smol",
      .symb_type = SB_VAL,
      .data_type = DT_UNIT,
    };

    foo_func = {
      .identifier = "foo",
      .symb_type = SB_FUNC,
      .data_type = DT_INTEGER,
      .param_count = 2,
      .param_type_arr = param_arr,
    };

    foo_main_func = {
      .identifier = "foo_main",
      .symb_type = SB_MAIN_FUNC, // special symbol type
      .data_type = DT_UNIT, // must be unit
      .param_count = 0, // must have no parameters
    };

    big_list_symbol = {
      .identifier = "BIG",
      .symb_type = SB_LIST,
      .data_type = DT_DOUBLE,
      .list_size = 11,
    };
  }
};

TEST_F(symbol_definitions, fixture_test) {

  // Initialized symbol table
  EXPECT_TRUE(st.list_first == NULL);
  EXPECT_TRUE(st.list_last == NULL);
  EXPECT_TRUE(st.stack_top == NULL);

  // Check random parts of symbols to see if they are real
  EXPECT_TRUE(x_val_int.identifier[0] == 'x');
  EXPECT_TRUE(x_var_double.symb_type == SB_VAR);
  EXPECT_TRUE(smol_val_unit.data_type == DT_UNIT);
  EXPECT_TRUE(foo_func.param_type_arr[1] == DT_DOUBLE);
  EXPECT_TRUE(big_list_symbol.list_size == 11);
}

TEST_F(symbol_definitions, define_symbol) {

  int ret = ST_enter_scope(&st);
  ASSERT_TRUE(ret == 0);

  // Insert symbol
  ret = ST_define_symbol(&st, &foo_main_func);
  ASSERT_TRUE(ret == 0);

  // Go deeper
  ret = ST_enter_scope(&st);
  ASSERT_TRUE(ret == 0);

  // Insert another symbol
  ret = ST_define_symbol(&st, &x_val_int);
  ASSERT_TRUE(ret == 0);

  // Attempt redefinition
  ret = ST_define_symbol(&st, &x_var_double);
  ASSERT_TRUE(ret == 4);

  // Shadowing
  ret = ST_enter_scope(&st);
  ASSERT_TRUE(ret == 0);
  ret = ST_define_symbol(&st, &x_var_double);
  ASSERT_TRUE(ret == 0);
}

TEST_F(symbol_definitions, get_symbol) {

  int ret = ST_enter_scope(&st);
  ASSERT_TRUE(ret == 0);
  ret = ST_define_symbol(&st, &foo_func);
  ASSERT_TRUE(ret == 0);

  // Get symbol
  Symbol * sb = ST_get_symbol(&st, foo_func.identifier);
  ASSERT_TRUE(sb != NULL);
  // Check uniq_id
  EXPECT_TRUE(sb->uniq_id == 1);

  // Main func
  ret = ST_define_symbol(&st, &foo_main_func);
  ASSERT_TRUE(ret == 0);

  sb = ST_get_symbol(&st, foo_main_func.identifier);
  ASSERT_TRUE(sb != NULL);
  // Check uniq_id - main must have uniq_id 0
  EXPECT_TRUE(sb->uniq_id == 0);

  // New block
  ret = ST_enter_scope(&st);
  ASSERT_TRUE(ret == 0);
  ret = ST_define_symbol(&st, &x_val_int);
  ASSERT_TRUE(ret == 0);

  // Get symbol
  sb = ST_get_symbol(&st, x_val_int.identifier);
  ASSERT_TRUE(sb != NULL);
  // Check uniq_id
  EXPECT_TRUE(sb->uniq_id == 2);

  // New block
  ret = ST_enter_scope(&st);
  ASSERT_TRUE(ret == 0);
  // Try shadowing
  ret = ST_define_symbol(&st, &x_val_int);
  ASSERT_TRUE(ret == 0);

  // Get symbol
  sb = ST_get_symbol(&st, x_val_int.identifier);
  ASSERT_TRUE(sb != NULL);
  // Check uniq_id
  EXPECT_TRUE(sb->uniq_id == 3);

  // We can access symbols further up the stack
  // Get symbol
  sb = ST_get_symbol(&st, foo_func.identifier);
  ASSERT_TRUE(sb != NULL);
  // Check uniq_id
  EXPECT_TRUE(sb->uniq_id == 1);

  // Try getting non-existent symbol
  sb = ST_get_symbol(&st, "blabla");
  ASSERT_TRUE(sb == NULL);

  // Exit block
  ST_exit_scope(&st);

  // Get symbol
  sb = ST_get_symbol(&st, x_val_int.identifier);
  ASSERT_TRUE(sb != NULL);
  // Check uniq_id - this should be the first "x"
  EXPECT_TRUE(sb->uniq_id == 2);

}
