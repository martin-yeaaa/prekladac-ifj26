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

void init_st() {
    symbol_table = ST_new();
}

int main()
{

    init_st();
    ST_enter_scope(&symbol_table);

    ST_destroy(&symbol_table);

    
    
    //printf("%d\n", new_symbol.id);

    return 0;
}