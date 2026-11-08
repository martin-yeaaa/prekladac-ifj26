// Project: prekladac-ifj26
// File: symtable.c
// Author: Martin Vaško (vaskom00)

// Implementation of symbol table functions

#include "symtable.h"
#include "bst.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

// Private structure declarations

// Structure for tables, with pointers to other tables
struct linked_node_struct {

  BSTNode table; // The table itself is a binary search tree

  // Pointers to other nodes
  // Next node in linked list (to keep reference)
  struct linked_node_struct *next_node;
  // Node on stack right below this node
  struct linked_node_struct *prev_scope;
};
typedef struct linked_node_struct *LNode;

// Structure for complete symbol tables
struct symbol_tables_struct {
  // There are 2 data structures
  // A linked list to keep a ptr to all tables
  LNode list_first;
  LNode list_last;
  // A stack to have correct shadowing behavior
  LNode stack_top;
};

// Function implementations

SymbTables ST_new() {

  SymbTables new_tables = {
      .list_first = NULL,
      .list_last = NULL,
      .stack_top = NULL,
  };

  return new_tables;
}

bool ST_enter_scope(SymbTables *tables) {

  assert(tables != NULL);

  // New BST table
  BSTNode new_table = BST_new();

  // Create new node
  LNode linked_node = malloc(sizeof(struct linked_node_struct));
  // Check memory allocation success
  if (linked_node == NULL) {
    return false;
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

  return true;
}

int ST_define_symbol(SymbTables *tables, SymbDraft *new_symbol) {

  // Assertions to prevent bugs
  assert(tables != NULL);
  assert(new_symbol != NULL);
  assert(new_symbol->identifier != NULL);
  if (new_symbol->param_count > 0) {
    assert(new_symbol->param_type_arr);
  }

  // Check if symbol is already defined
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
