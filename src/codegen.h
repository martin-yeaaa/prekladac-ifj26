// Project: prekladac-ifj26
// File: codegen.h
// Author: Lukáš Dvořák (xdvoral00)

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast_module.h"

// Main function to generate IFJcode26 from AST
// No '*' here because ASTNode is already a pointer in semantics.h
void generate_code(ASTNode root);

#endif