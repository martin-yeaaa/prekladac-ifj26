// Project: prekladac-ifj26
// File: expr_parser.h
// Author: Martin Vaško (xvaskom00)

// Header file for expression parser functions

#ifndef EXPR_PARSER_H
#define EXPR_PARSER_H

#include "ast_module.h"
#include "error_codes.h"
#include "lexer.h"

#include <stdbool.h>

/**
 * @brief Parses expressions (including conditions) using
 * precedence syntax analysis.
 * @param parent AST node which has the expression as a child node.
 * @param cur_token_ptr Pointer to pointer to currently parsed token.
 * @param is_cond Set to `true` if expression is condition,
 * otherwise set to `false`.
 * @return `SUCCESS` if expression has correct syntax, otherwise `ERR_SYNTAX`,
 * or `ERR_INTERNAL` on memory failure.
 */
ErrCode parse_expr(ASTNode parent, Token **cur_token_ptr, bool is_cond);

#endif
