// Project: prekladac-ifj26
// File: lexer.h
// Author: Michal Bína (xbinami00), Jiří Švec (xsvecji00)

// Header for lexer

#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include "bst.h"

typedef enum token_type{
    IDENTIFIER,

    // Values
    VAL_INT,
    VAL_DOUBLE,
    VAL_STRING,
    VAL_BOOL,

    // Required keywords
    KW_DEF,
    KW_DO,
    KW_DOUBLE,
    KW_ELSE,
    KW_IF,
    KW_IMPORT,
    KW_INT,
    KW_RETURN,
    KW_STRING,
    KW_THEN,
    KW_UNIT,
    KW_VAL,
    KW_VAR,
    KW_WHILE,

    // Keywords from extensions
    KW_BOOLEAN,
    KW_CASE,
    KW_CLASS,
    KW_EXTENSION,
    KW_FOR,
    KW_LIST,
    KW_MATCH,
    KW_TYPE,

    // Operators
    OP_PLUS,
    OP_MINUS,
    OP_MULT,
    OP_DIV,
    OP_LESS,
    OP_LESS_EQ,
    OP_GREATER,
    OP_GREATER_EQ,
    OP_EQ,
    OP_NOT_EQ,

    // Separators
    ASSIGNMENT,
    COMMA,
    SEMICOLON,
    AT,
    COLON,
    SPACE,
    INDENT,
    NEW_LINE,
    END_OF_FILE,
    OPEN_BRACKET,
    CLOSE_BRACKET
} token_type_t;

typedef struct token
{
    token_type_t type;
    char *value;
    struct token *next;
} token_t;

/**
 * @brief Reads whole standard input and stores it in newly allocated string.
 * @return Newly allocated string with the input.
 */
char *read_stdin(void);

/**
 * @brief Returns next token from the list created by lexer_parse().
 * @return Next token, END_OF_FILE token repeatedly at the end, `NULL` if there is no list.
 */
token_t *get_next_token(void);

/**
 * @brief Appends new token to the end of token list.
 * @param type Type of token.
 * @param value Value of token, the function stores its own copy.
 */
void lexer_push(token_type_t type, const char *value);

/**
 * @brief Frees the whole token list.
 * @param head Pointer to first token of the list.
 */
void lexer_free_tokens(token_t *head);

/**
 * @brief Inserts all keywords into the tree.
 * @param keyword_tree Pointer to the tree root.
 */
void fill_keyword_tree(BSTNode *keyword_tree);

/**
 * @brief Splits the source code into list of tokens.
 * @param source String containing source code.
 * @return First token of the list, last one is END_OF_FILE.
 */
token_t *lexer_parse(char *source);

#endif // LEXER_H
