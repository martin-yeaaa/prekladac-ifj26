// Project: prekladac-ifj26
// File: parser_priv.h
// Author: Martin Vaško (vaskom00)

#ifndef PARSER_PRIV_H
#define PARSER_PRIV_H

#include "error_codes.h"
#include "lexer.h"
#include "semantics.h"

/**
 * @brief Creates a new AST node and assigns it to parent. It only serves as a
 * helper function to call both AST_init_node() and AST_node_add_child() in one
 * call, and properly free memory on error.
 * @param parent Initialized ASTNode, which will be the parent.
 * @param ptr_uninit Pointer to declared uninitialized node, which will be the
 * child.
 * @param node_type What the child node represents.
 * @param data_type Type of data the child node holds.
 * @param node_data Value of literal, or identifier string.
 * This is ignored when it's not needed.
 * @return `SUCCESS` on success,
 * `ERR_INTERNAL` on memory allocation failure,
 * `ERR_INCOMPATIBLE_TYPE` if conversion fails while assigning data.
 */
static ErrCode make_child_node(ASTNode parent, ASTNode *ptr_uninit,
                               NodeType n_type, DataType d_type, char *n_data);

/**
 * @brief Syntax analysis functions parse_x. Each function matches
 * one grammar rule.
 * @param ast_node AST parent node of currently analyzed rule.
 * @return `SUCCESS` if program has no syntax errors, `ERR_SYNTAX` otherwise.
 */
static ErrCode parse_program(ASTNode ast_root);
/**
 * @param cur_token Currently processed token
 */
static ErrCode parse_prolog(void);
static ErrCode parse_fun_def_list(ASTNode ast_root);
static ErrCode parse_fun_def_any(ASTNode ast_root);
static ErrCode parse_fun_def(ASTNode func_def);

static ErrCode parse_param_list(ASTNode func_def);
static ErrCode parse_param_sep(ASTNode param_list);
static ErrCode parse_param(ASTNode param_list);

static ErrCode parse_data_type_opt(ASTNode idtf, ASTNode extra_node);
static ErrCode parse_data_type(ASTNode idtf);

static ErrCode parse_fun_body(ASTNode func_def);
static ErrCode parse_expr_or_call(ASTNode parent);
static ErrCode parse_block(ASTNode parent);

static ErrCode parse_stmts(ASTNode block);
static ErrCode parse_stmt_sep(ASTNode block);

static ErrCode parse_stmt(ASTNode block);
static ErrCode parse_stmt_next(ASTNode block);
static ErrCode parse_decision(ASTNode block);

// static ErrCode parse_stmt_ident_rest(void);

static ErrCode parse_val_def(ASTNode block);
static ErrCode parse_var_def(ASTNode block);
static ErrCode parse_assignment(ASTNode block);

// static ErrCode parse_expr_or_call_rest(void);

static ErrCode parse_if_stmt(ASTNode block);
static ErrCode parse_while_loop(ASTNode block);
static ErrCode parse_cond(ASTNode parent);

static ErrCode parse_return_stmt(ASTNode block);
static ErrCode parse_fun_call(ASTNode parent);

static ErrCode parse_arg_list(ASTNode func_call);
static ErrCode parse_arg_sep(ASTNode arg_list);
static ErrCode parse_arg(ASTNode arg_list);

static ErrCode parse_unary_op(ASTNode *ptr_to_node);
static ErrCode parse_literal(ASTNode parent);

static ErrCode parse_expr_un(void);
static ErrCode parse_expr_un_opt(void);
static ErrCode parse_expr(ASTNode parent);
static ErrCode parse_simple_literal(void);
static ErrCode parse_paren_tail(void);
static ErrCode parse_expr_rest(void);

static ErrCode parse_binary_op(void);

static ErrCode parse_rel_op(void);

#endif
