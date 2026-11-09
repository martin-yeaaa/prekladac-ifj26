// Project: prekladac-ifj26
// File: symtable.c
// Author: Martin Vaško (vaskom00)

// Implementation of symbol table functions

#include "symtable.h"
#include "bst.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

// Function implementations

SymbTables ST_new() {

  SymbTables new_tables = {
      .list_first = NULL,
      .list_last = NULL,
      .stack_top = NULL,
  };

  return new_tables;
}

int ST_enter_scope(SymbTables *tables) {

  assert(tables != NULL);

  // New BST table
  BSTNode new_table = BST_new();

  // Create new node
  LNode linked_node = malloc(sizeof(struct linked_node_struct));
  // Check memory allocation success
  if (linked_node == NULL) {
    return 99;
  }

  // Initialize structure
  linked_node->table = new_table;
  // List pointer
  linked_node->next_node = NULL;
  // Stack pointer
  linked_node->prev_scope = tables->stack_top;

  // Insert node into main struct
  // Check if it's the first node
  if (tables->list_first == NULL) {
    tables->list_first = linked_node;
  }
  // If it's not first, link list nodes
  else {
    tables->list_last->next_node = linked_node;
  }

  // Set the rest of the pointers
  tables->list_last = linked_node;
  tables->stack_top = linked_node;

  return 0;
}

void ST_exit_scope(SymbTables *tables) {

  // Assertions
  assert(tables != NULL);
  // This really shouldn't be called on an empty stack
  assert(tables->stack_top != NULL);

  // Pop table from stack, keep it in list
  tables->stack_top = tables->stack_top->prev_scope;
}

int ST_define_symbol(SymbTables *tables, SymbDraft *symbol_draft) {

  // Assertions to prevent bugs
  assert(tables != NULL);
  assert(tables->stack_top != NULL); // must be in some scope!
  assert(symbol_draft != NULL);
  assert(symbol_draft->identifier != NULL);
  if (symbol_draft->param_count > 0) {
    assert(symbol_draft->param_type_arr);
  }

  // Check if symbol is already defined
  if (BST_get_data(tables->stack_top->table, symbol_draft->identifier) !=
      NULL) {
    // Error, symbol is already defined!!
    return 4;
  }

  // Allocate memory for symbol (it has flexible array member)
  size_t symbol_size =
      sizeof(Symbol) + sizeof(DataType) * symbol_draft->param_count;
  Symbol *new_symbol = malloc(symbol_size);
  // Check allocation success
  if (new_symbol == NULL) {
    // Allocation failed :(
    return 99;
  }

  // Initialize symbol
  new_symbol->symb_type = symbol_draft->symb_type;
  new_symbol->data_type = symbol_draft->data_type;
  new_symbol->list_size = symbol_draft->list_size;
  new_symbol->param_count = symbol_draft->param_count;
  // Copy parameter array
  for (int i = 0; i < symbol_draft->param_count; i++) {
    new_symbol->param_type_arr[i] = symbol_draft->param_type_arr[i];
  }

  // Static counter of inserted symbols used for setting unique ids
  static int symbol_counter = 0;

  // Set unique integer id for code generator
  if (new_symbol->symb_type == SB_MAIN_FUNC) {
    // id 0 is reserved for main function
    new_symbol->uniq_id = 0;
  } else {
    new_symbol->uniq_id = symbol_counter + 1;
    symbol_counter += 1; // Increment counter
  }

  BSTNode *current_scope = &tables->stack_top->table;
  BSTKey identifier = symbol_draft->identifier;
  // Insert symbol into table
  bool insert = BST_insert(current_scope, identifier, new_symbol, symbol_size);
  if (insert == false) {
    // Insertion failed == memory allocation failed
    free(new_symbol);
    return 99;
  }

  // Tree makes its own copy of data, we can free this
  free(new_symbol);

  // Success :3
  return 0;
}

Symbol *ST_get_symbol(SymbTables *tables, BSTKey id) {

  assert(tables != NULL);
  assert(id != NULL);

  // Check stack of symbol tables
  LNode closest_node = tables->stack_top;
  Symbol *defined_symbol = NULL;

  // Look for symbol until it's found or until
  // we run out of tables on the stack
  while (defined_symbol == NULL && closest_node != NULL) {

    // Search for symbol
    defined_symbol = BST_get_data(closest_node->table, id);
    // Move up the stack
    closest_node = closest_node->prev_scope;
  }

  return defined_symbol;
}

void ST_destroy(SymbTables *tables) {

  assert(tables != NULL);

  // Keep a pointer to next undeleted node with table
  LNode next_to_del = tables->list_first;

  // While tables exist, keep freeing memory
  while (next_to_del != NULL) {

    // Save the pointer to the node to delete now
    LNode del_now = next_to_del;

    // Set next undeleted node while we have the pointer
    next_to_del = next_to_del->next_node;

    // Free memory
    BST_destroy_tree(del_now->table);
    free(del_now);
  }
}
