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
  S_MAIN_FUNC, // Use for function with @main
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
// Cannot be private because of type checking (probably)
struct symbol_struct {

  SymbType symb_type;
  DataType data_type;
  int id;
  int list_size;
  int param_count;
  DataType param_type_arr[]; // Flexible array member
};
typedef struct symbol_struct Symbol;

// Symbol tables typedef, private structure
typedef struct symbol_tables_struct SymbTables;

// Structure for incomplete symbols, public!
struct symbol_draft_struct {

  BSTKey identifier; // String
  SymbType symb_type;
  DataType data_type;
  int list_size;            // If the symbol is a list, set the size
  int param_count;          // If the symbol is a function, set parameter count
  DataType *param_type_arr; // Array of parameter types (if symbol is function)
};
typedef struct symbol_draft_struct SymbDraft;

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


/**
 * Creates a new entry in the symbol table on the top of stack.
 * @param tables Pointer to `SymbTables` structure.
 * @param symbol_draft Pointer to symbol draft structure. This structure must be
 * properly initialized manually before calling this function.
 * @return `0` if no errors occur, `4` if symbol is already defined,
 * `99` if memory allocation fails.
 */
int ST_define_symbol(SymbTables *tables, SymbDraft *symbol_draft);

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


/*
My note:

When defining symbol, you need to know the amount of parameters because of
flexible array member in `Symbol`. This implies that the struct `Symbol` cannot
be used until the entire symbol definition was parsed.

First we make AST with identifier strings, and at that point we
know everything about the symbol, probably, we just read the nodes i think
*/