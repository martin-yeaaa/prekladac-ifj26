// Project: prekladac-ifj26
// File: symtable.h
// Author: Martin Vaško (vaskom00)

// Symbol table interface

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "bst.h"
#include "error_codes.h"
#include <stdbool.h>

// Enum for different types of symbols
typedef enum symbol_type_enum {
  SB_VAR,
  SB_VAL,
  SB_FUNC,
  SB_MAIN_FUNC, // Use for function with @main
  SB_BUILTIN,   // Built-in functions
} SymbType;

// Enum for data type of symbol or of function return value
// This enum is also used by AST
typedef enum data_type_enum {
  DT_INTEGER,
  DT_DOUBLE,
  DT_STRING,
  DT_UNIT,
  DT_BOOL,
  DT_ANY, // ONLY FOR BUILT-IN FUNCTIONS!
  DT_UNDEF
} DataType;

// Structure for incomplete symbols
struct symbol_draft_struct {

  BSTKey identifier; // String
  SymbType symb_type;
  DataType data_type;
  int param_count;          // If the symbol is a function, set parameter count
  DataType *param_type_arr; // Array of parameter types (if symbol is function)
};
typedef struct symbol_draft_struct SymbDraft;

// Structure for symbols
struct symbol_struct {

  SymbType symb_type;
  DataType data_type;
  int uniq_id;
  int param_count;
  DataType param_type_arr[]; // Flexible array member
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
 * Creates a new structure for symbol tables, enters global scope,
 * and prefills it with symbols of built-in functions.
 * @param tables Pointer to declared uninitialized `SymbTables` structure.
 * Will be initialized and modified. Must not be `NULL`.
 * @return `SUCCESS` if no errors occur,
 * `ERR_INTERNAL` if memory allocation fails.
 */
ErrCode ST_init(SymbTables *tables);

/**
 * Creates a new table. Call this after creating a new SymbTables structure or
 * when entering a new indented block.
 * @param tables Pointer to `SymbTables` structure.
 * @return `SUCCESS` if no errors occur,
 * `ERR_INTERNAL` if memory allocation fails.
 */
ErrCode ST_enter_scope(SymbTables *tables);

/**
 * Pops table from the top of symbol table stack. Call this when exiting an
 * indented block.
 * @param tables Pointer to `SymbTables` structure.
 */
void ST_exit_scope(SymbTables *tables);

/**
 * Creates a new entry in the symbol table on the top of stack.
 * @param tables Pointer to `SymbTables` structure.
 * @param symbol_draft Pointer to symbol draft structure. This structure must be
 * properly initialized manually before calling this function.
 * @return `SUCCESS` if no errors occur,
 * `ERR_REDEFINITION` if symbol is already defined,
 * `ERR_INTERNAL` if memory allocation fails.
 */
ErrCode ST_define_symbol(SymbTables *tables, SymbDraft *symbol_draft);

/**
 * Finds the symbol closest to the top of symbol table stack.
 * @param tables Pointer to `SymbTables` structure.
 * @param id String identifier of symbol.
 * @return Pointer to `Symbol` if symbol is defined, otherwise `NULL`.
 */
Symbol *ST_get_symbol(SymbTables *tables, BSTKey id);

/**
 * Removes all tables from memory. After this, `tables` becomes unusable.
 * @param tables Pointer to `SymbTables` structure.
 */
void ST_destroy(SymbTables *tables);

#endif // SYMTABLE_H
