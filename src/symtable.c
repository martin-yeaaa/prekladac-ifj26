// Project: prekladac-ifj26
// File: symtable.c
// Author: Martin Vaško (vaskom00)

// Implementation of symbol table functions

#include "symtable.h"
#include "bst.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

SymbTables ST_new() {

  SymbTables new_tables = {
      .list_first = NULL,
      .list_last = NULL,
      .stack_top = NULL,
  };

  return new_tables;
}

bool ST_enter_scope(SymbTables *tables) {

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

void ST_destroy(SymbTables *tables) {

  // Keep a pointer to next undeleted node with table
  LNode next_to_del = tables->list_first;

  // While tables exist, keep freeing memory
  while (next_to_del != NULL) {

    // Save the pointer to the node to delete now
    LNode del_now = next_to_del;

    // Set next undeleted node while we have the pointer
    next_to_del = next_to_del->next_node;

    BST_destroy_tree(del_now->table);
    free(del_now);
  }
}
