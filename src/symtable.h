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
typedef struct symbol_struct *Symbol;

// Structure for tables, with pointer to another table
struct linked_table_struct {
  BSTNode table;
  struct linked_table_struct *next;
};
typedef struct linked_table_struct *LTable;

// List of all tables, only serves to keep a reference ptr to all tables
struct tables_list_struct {
  LTable first;
  LTable last;
};
typedef struct tables_list_struct *STList;

// Stack of tables, used for correct shadowing behavior
struct tables_stack_struct {
  LTable top;
};
typedef struct tables_stack_struct *STStack;

// Structure for complete symbol tables
struct symbol_tables_struct {
  STList st_list;
  STStack st_stack;
};
typedef struct symbol_tables_struct SymbTable;

SymbTable ST_new();

Symbol create_symbol(SymbType s, DataType d);

Symbol ST_get_symbol(BSTKey id);
// if token is kw_val or kw_var, we are initializing a new variable/const
bool ST_add_local_symbol(BSTKey id, Symbol symb);

bool ST_add_global_symbol(BSTKey id, Symbol symb);
