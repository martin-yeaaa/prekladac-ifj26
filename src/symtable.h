// Project: prekladac-ifj26
// File: symtable.h
// Author: Martin Vaško (vaskom00)

// Symbol table interface

#include "bst.h"
#include <stdbool.h>

// Enum for different types of symbols
typedef enum symbol_type_enum {
  S_VAR,
  S_VAL,
  S_FUNC,
  S_LIST,
} SymbType;

// Enum for data type of symbol or of function return value
typedef enum data_type_enum {
  DT_INTEGER,
  DT_DOUBLE,
  DT_STRING,
  DT_UNIT,
  DT_BOOL,
  DT_UNDEF
} DataType;

// Structure for symbols
struct symbol_struct {

  SymbType symb_type;
  DataType data_type;
  int id;
  int param_count;
  DataType param_type[]; // Flexible array member
};
typedef struct symbol_struct Symbol;

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
typedef struct symbol_tables_struct SymbTables;

/**
 * Creates a new structure for symbol tables.
 * @return Empty symbol table structure.
 */
SymbTables ST_new();

/**
 * Creates a new table. Call this after creating a new SymbTables structure or
 * when entering a new indented block.
 * @param tables Pointer to `SymbTables` structure.
 * @return `true` if memory allocation succeeds, otherwise false.
 */
bool ST_enter_scope(SymbTables *tables);

Symbol ST_create_symbol(SymbType s, DataType d);

Symbol ST_get_symbol(BSTKey id);
// if token is kw_val or kw_var, we are initializing a new variable/const
bool ST_add_local_symbol(BSTKey id, Symbol symb);

bool ST_add_global_symbol(BSTKey id, Symbol symb);
