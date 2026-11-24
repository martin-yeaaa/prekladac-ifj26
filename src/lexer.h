// Project: prekladac-ifj26
// File: lexer.h
// Authors:
// - Jiří Švec (xsvecji00)
// - Michal Bína (xbinami00)
// - Martin Vaško (xvaskom00) - only final touches

// Header for lexer

#ifndef LEXER_H
#define LEXER_H

typedef enum token_type_enum {
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
  OP_EQ, // ==
  OP_NOT_EQ,

  // Separators
  ASSIGNMENT, // =
  COMMA,
  SEMICOLON,
  AT_MAIN,
  COLON,
  DOT,
  INDENT,
  DEDENT,
  NEW_LINE,
  END_OF_FILE,
  OPEN_BRACKET,
  CLOSE_BRACKET
} TokenType;

typedef struct token_struct {
  TokenType type;
  char *value;
  struct token_struct *next;
} Token;

/**
 * @brief Reads whole standard input and splits it into list of tokens.
 * @return First token of the list, last one is END_OF_FILE.
 */
Token *lexer_parse(void);

/**
 * @brief Frees the whole token list.
 * @param head Pointer to first token of the list.
 */
void lexer_free_tokens(Token *head);

/**
 * @brief Returns next token from the list created by lexer_parse().
 * @return Next token, END_OF_FILE token repeatedly at the end, `NULL` if there
 * is no list.
 */
Token *get_next_token(void);

#endif // LEXER_H
