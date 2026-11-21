// Project: prekladac-ifj26
// File: parser.h
// Author: Martin Vaško (vaskom00)

#ifndef PARSER_H
#define PARSER_H

#include "error_codes.h"

/**
 * @brief Compiles program. Input program is in language IFJ26. Output program
 * is in language IFJcode26. Input is read from stdin. Output is written into
 * stdout.
 * @return `SUCCESS` if program successfuly compiles, otherwise one of the
 * error codes, depending on what kind of error the program has.
 */
ErrCode compile(void);

static ErrCode parse_program(ASTNode *ast);
static ErrCode parse_prolog(void);
static ErrCode parse_fun_def_list(void);
static ErrCode parse_fun_def_any(void);
static ErrCode parse_fun_def(void);
static ErrCode parse_param_list(void);
static ErrCode parse_param_sep(void);
static ErrCode parse_param(void);
static ErrCode parse_data_type_opt(void);
static ErrCode parse_data_type(void);
static ErrCode parse_fun_body(void);
static ErrCode parse_block(void);
static ErrCode parse_stmts(void);
static ErrCode parse_stmt_sep(void);
static ErrCode parse_stmt_next(void);
static ErrCode parse_stmt(void);
static ErrCode parse_stmt_ident_rest(void);
static ErrCode parse_return_stmt(void);
static ErrCode parse_val_def(void);
static ErrCode parse_var_def(void);
static ErrCode parse_expr_or_call(void);
static ErrCode parse_expr_or_call_rest(void);
static ErrCode parse_arg_list(void);
static ErrCode parse_arg_sep(void);
static ErrCode parse_arg(void);
static ErrCode parse_expr_un(void);
static ErrCode parse_expr_un_opt(void);
static ErrCode parse_expr(void);
static ErrCode parse_simple_literal(void);
static ErrCode parse_paren_tail(void);
static ErrCode parse_expr_rest(void);
static ErrCode parse_unary_op(void);
static ErrCode parse_binary_op(void);
static ErrCode parse_if_stmt(void);
static ErrCode parse_while_loop(void);
static ErrCode parse_cond(void);
static ErrCode parse_rel_op(void);
static ErrCode parse_literal(void);

#endif