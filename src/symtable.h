// Project: prekladac-ifj26
// File: symtable.h
// Author: Martin Vaško (vaskom00)

// Symbol table interface

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

typedef struct symbol_struct {

  SymbType symb_type;
  DataType data_type;
  bool modifiable;
  bool was_defined;
  int param_count;
  DataType param_type[];

} Symbol;

void ST_new();

// if token is kw_val or kw_var, we are initializing a new variable/const
void ST_add_symbol(char *symbol);
