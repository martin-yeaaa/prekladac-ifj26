// Project: prekladac-ifj26
// File: expr_parser.h
// Author: Martin Vaško (xvaskom00)

// Header file for expression parser functions

#ifndef EXPR_PARSER_H
#define EXPR_PARSER_H

#include "ast_module.h"
#include "error_codes.h"
#include "lexer.h"

ErrCode parse_expr(ASTNode parent, Token **cur_token_ptr);
ErrCode parse_cond(ASTNode parent, Token **cur_token_ptr);

#endif
