// Project: prekladac-ifj26
// File: semantics.h
// Author: Martin Vaško (xvaskom00)

// Header file for semantic analysis

#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "ast_module.h"
#include "error_codes.h"
#include "symtable.h"

/**
 * @brief Fills the symbol table, does all semantic checks.
 * @param root Root of AST.
 * @param st Pointer to INITIALIZED symbol tables structure.
 * @return 'SUCCESS' if analysis is successful, otherwise one of the following
 * error codes: `ERR_UNDEF_SYM`, `ERR_REDEFINITION`, `ERR_FUN_ARGS`,
 * `ERR_INCOMPATIBLE_TYPE`, `ERR_INVALID_ASSIGN`, `ERR_OTHER_SEMANTIC`,
 * `ERR_INTERNAL`
 */
ErrCode semantic_analysis(ASTNode root, SymbTables *st);

#endif
