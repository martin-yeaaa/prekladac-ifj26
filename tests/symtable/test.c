// Project: prekladac-ifj26
// File: test.c
// Author: Martin Vaško (xvaskom00)

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/symtable.h"

/*
My note, ignore this ig:

When defining symbol, you need to know the amount of parameters because of
flexible array member in `Symbol`. This implies that the struct `Symbol` cannot
be used until the entire symbol definition was parsed.

First we make AST with identifier strings, and at that point we
know everything about the symbol, probably, we just read the nodes i think
*/


// Global symbol table structure
SymbTables symbol_table;


int main()
{

    // Initialize table
    int err = ST_init(&symbol_table);
    if (err == 99) exit(err);

    // Global scope is already entered !
    
    // DONT DO THIS:
    // err = ST_enter_scope(&symbol_table);
    // Check success
    // if (err == 99) exit(err);

    
    // Copied structures from symtable_test.cc

    // Main function
    SymbDraft foo_main_func = {
      .identifier = "foo_main",
      .symb_type = SB_MAIN_FUNC, // special symbol type
      .data_type = DT_UNIT, // must be unit
      .param_count = 0, // must have no parameters
    };

    // parameters arr
    DataType param_arr[2] = {DT_INTEGER, DT_DOUBLE};

    // another function
    SymbDraft foo_func = {
      .identifier = "foo",
      .symb_type = SB_FUNC,
      .data_type = DT_INTEGER,
      .param_count = 2,
      .param_type_arr = param_arr,
    };

    // Define symbols
    err = ST_define_symbol(&symbol_table, &foo_main_func);
    // Check definition success
    if (err == 4 || err == 99) exit(err);

    err = ST_define_symbol(&symbol_table, &foo_func);
    if (err == 4 || err == 99) exit(err);

    // Try to get symbol
    Symbol * yay = ST_get_symbol(&symbol_table, "foo");
    if (yay != NULL)
        printf("foo has %d parameters\n", yay->param_count);
    else
        printf("foo not found\n");
    
    // Enter local scope
    err = ST_enter_scope(&symbol_table);
    if (err == 99) exit(err);

    // More symbol drafts copied from symtable_test.cc
    SymbDraft x_var_double = {
      .identifier = "x",
      .symb_type = SB_VAR,
      .data_type = DT_DOUBLE,
    };

    // Define symbols
    err = ST_define_symbol(&symbol_table, &x_var_double);
    // Check definition success
    if (err == 4 || err == 99) exit(err);

    // Get symbol x
    Symbol * get_x_1 = ST_get_symbol(&symbol_table, "x");
    if (get_x_1 != NULL)
        printf("x has id %d\n", get_x_1->uniq_id);
    else
        printf("x not found\n");

    // Redefinition is not allowed
    err = ST_define_symbol(&symbol_table, &x_var_double);
    if (err == 99) exit(err);
    if (err == 4) fprintf(stderr, "Redefinition of x !!! BAD!!!\n");

    // Example continues

    // Enter local scope
    err = ST_enter_scope(&symbol_table);
    if (err == 99) exit(err);

    // Now we can shadow x
    err = ST_define_symbol(&symbol_table, &x_var_double);
    if (err == 99 || err == 4) exit(err);

    Symbol * get_x_2 = ST_get_symbol(&symbol_table, "x");
    if (get_x_2 != NULL)
        printf("x has id %d\n", get_x_2->uniq_id);
    else
        printf("x not found\n");

    // Exit scope
    ST_exit_scope(&symbol_table);
    // Now we should get the first x symbol again
    Symbol * get_x_3 = ST_get_symbol(&symbol_table, "x");
    if (get_x_3 != NULL)
        printf("x has id %d\n", get_x_3->uniq_id);
    else
        printf("x not found\n");
    

    // After using symbol tables, free them from memory
    ST_destroy(&symbol_table);

    // Run valgrind to check memory leaks

    return 0;
}