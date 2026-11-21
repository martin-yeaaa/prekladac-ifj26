// Project: prekladac-ifj26
// File: lexer.c
// Author: Michal Bína (xbinami00), Jiří Švec (xsvecji00)

// Implementation of lexer

#include "lexer.h"
#include "bst.h"
#include "error_codes.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static token_t *list;        // last token in the list
static token_t *list_head;   // empty token at the start of list
static token_t *next_token;  // token that get_next_token() returns next
static char *lexer_source;   // whole stdin stored in string
static size_t *indent_stack; // indentation levels (number of spaces), first one
                             // is always 0
static BSTNode keyword_tree;

void lexer_free_tokens(token_t *head) {
  next_token = NULL;

  while (head != NULL) {
    token_t *next = head->next;
    free(head->value);
    free(head);
    head = next;
  }
}

/**
 * @brief Frees token list, source code, indentation stack and keyword tree.
 */
static void lexer_cleanup(void) {
  lexer_free_tokens(list_head);
  list_head = NULL;
  list = NULL;

  free(lexer_source);
  lexer_source = NULL;

  free(indent_stack);
  indent_stack = NULL;

  if (keyword_tree != NULL) {
    BST_destroy_tree(keyword_tree);
    keyword_tree = NULL;
  }
}

/**
 * @brief Prints an error to stderr, frees memory and exits.
 * @param code Exit code, one of the macros from error_codes.h.
 * @param message Error description.
 */
static void lexer_error(ErrCode code, const char *message) {
  fprintf(stderr, "Lexer error: %s\n", message);
  lexer_cleanup();
  exit(code);
}

/**
 * @brief Creates a copy of part of a string.
 * @param src Start of the copied text.
 * @param length Number of copied characters.
 * @return New copy of string.
 */
static char *copy_string(const char *src, size_t length) {
  char *copy = malloc(length + 1);
  if (copy == NULL)
    lexer_error(ERR_INTERNAL, "memory allocation failed");

  memcpy(copy, src, length);
  copy[length] = '\0';
  return copy;
}

/**
 * @brief Checks whether character is a decimal digit.
 * @param c Character to check.
 * @return `true` for '0'-'9'.
 */
static bool is_digit(char c) { return c >= '0' && c <= '9'; }

/**
 * @brief Checks if a character is in English alphabet.
 * @param c Character to check.
 * @return `true` for 'a'-'z' and 'A'-'Z'.
 */
static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/**
 * @brief Translates the character after a backslash.
 * @param c Character following the backslash.
 * @return Decoded character or -1 if invalid.
 */
static int decode_escape(char c) {
  switch (c) {
  case '\\':
    return '\\';
  case '"':
    return '"';
  case 'n':
    return '\n';
  case 'r':
    return '\r';
  case 't':
    return '\t';
  default:
    return -1;
  }
}

/**
 * @brief Skips digits with optional underscores between them.
 * @param source Source code.
 * @param length Length of the source.
 * @param i Position of the first digit.
 * @return Position after the last digit.
 */
static size_t skip_digits(const char *source, size_t length, size_t i) {
  if (i >= length || !is_digit(source[i]))
    lexer_error(ERR_LEXICAL, "digit expected in number literal");

  while (i < length && (is_digit(source[i]) || source[i] == '_'))
    i++;

  if (source[i - 1] == '_')
    lexer_error(ERR_LEXICAL, "underscore must be between two digits");

  return i;
}

/**
 * @brief Skips a (nested) block comment.
 * @param source Source code.
 * @param length Length of the source.
 * @param i Position of the first '/' of the comment.
 * @param new_line Set to `true` if the comment contains a new line.
 * @return Position after the end of the comment.
 */
static size_t skip_block_comment(const char *source, size_t length, size_t i,
                                 bool *new_line) {
  i += 2;
  size_t depth = 1; // comments can be inside of other comments
  *new_line = false;

  while (i < length && depth > 0) {
    if (source[i] == '\n') {
      *new_line = true;
      i++;
    } else if (i + 1 < length && source[i] == '/' && source[i + 1] == '*') {
      depth++;
      i += 2;
    } else if (i + 1 < length && source[i] == '*' && source[i + 1] == '/') {
      depth--;
      i += 2;
    } else {
      i++;
    }
  }

  if (depth != 0)
    lexer_error(ERR_LEXICAL, "unterminated block comment");

  return i;
}

/**
 * @brief Checks whether the current logical line already contains a token.
 * @return `true` if the last token is not NEW_LINE, INDENT or DEDENT.
 */
static bool line_has_token(void) {
  return list != list_head && list->type != NEW_LINE && list->type != INDENT &&
         list->type != DEDENT;
}

/**
 * @brief Reads whole standard input and stores it in newly allocated string.
 * @return Newly allocated string with the input.
 */
static char *read_stdin(void) {
  size_t capacity = 4096;
  size_t size = 0;

  char *buffer = malloc(capacity);
  if (buffer == NULL)
    lexer_error(ERR_INTERNAL, "memory allocation failed");

  int c;
  while ((c = getchar()) != EOF) {
    // buffer is full (we need 1 byte for '\0'), so we need to make it 2x bigger
    if (size + 1 >= capacity) {
      capacity *= 2;
      char *tmp = realloc(buffer, capacity);
      if (tmp == NULL) {
        free(buffer);
        lexer_error(ERR_INTERNAL, "memory allocation failed");
      }
      buffer = tmp;
    }

    buffer[size++] = (char)c;
  }

  if (ferror(stdin)) {
    free(buffer);
    lexer_error(ERR_INTERNAL, "reading from stdin failed");
  }

  buffer[size] = '\0';
  return buffer;
}

token_t *get_next_token(void) {
  token_t *token = next_token;

  if (token != NULL && token->next != NULL)
    next_token = token->next;

  return token;
}

void lexer_push(token_type_t type, const char *value) {
  token_t *token = calloc(1, sizeof(token_t));
  if (token == NULL)
    lexer_error(ERR_INTERNAL, "memory allocation failed");

  token->type = type;
  token->value = copy_string(value, strlen(value));

  list->next = token;
  list = token;
}

void fill_keyword_tree(BSTNode *keyword_tree) {
  /*
   * AI helped here to complete keyword list so we did not have to
   * fill it by hand.
   *
   * Prompt:
   *
   * "tady je seznam klíčových slov:
   * def do Double else if import Int return
   * String then Unit val var while
   * Boolean case class extension for List match type
   *
   * tady je seznam keywordů:
   * // Required keywords
   * KW_DEF,
   * KW_DO,
   * KW_DOUBLE,
   * KW_ELSE,
   * KW_IF,
   * KW_IMPORT,
   * KW_INT,
   * KW_RETURN,
   * KW_STRING,
   * KW_THEN,
   * KW_UNIT,
   * KW_VAL,
   * KW_VAR,
   * KW_WHILE,

   * // Keywords from extensions
   * KW_BOOLEAN,
   * KW_CASE,
   * KW_CLASS,
   * KW_EXTENSION,
   * KW_FOR,
   * KW_LIST,
   * KW_MATCH,
   * KW_TYPE,
   *
   * potřebuju, abys mi to doplnil zbytek do tohoto pole:
   * struct {
   *  const char *str;
   *  token_type_t type;
   * } keywords[] = {
   * { "def",     KW_DEF },
   * { "do",      KW_DO },
   * { "Double",  KW_DOUBLE };
   * "
   */

  struct {
    const char *str;
    token_type_t type;
  } keywords[] = {{"def", KW_DEF},         {"do", KW_DO},
                  {"Double", KW_DOUBLE},   {"else", KW_ELSE},
                  {"if", KW_IF},           {"import", KW_IMPORT},
                  {"Int", KW_INT},         {"return", KW_RETURN},
                  {"String", KW_STRING},   {"then", KW_THEN},
                  {"Unit", KW_UNIT},       {"val", KW_VAL},
                  {"var", KW_VAR},         {"while", KW_WHILE},

                  {"Boolean", KW_BOOLEAN}, {"case", KW_CASE},
                  {"class", KW_CLASS},     {"extension", KW_EXTENSION},
                  {"for", KW_FOR},         {"List", KW_LIST},
                  {"match", KW_MATCH},     {"type", KW_TYPE},

                  {"true", VAL_BOOL},      {"false", VAL_BOOL}};

  size_t count = sizeof(keywords) / sizeof(keywords[0]);

  for (size_t i = 0; i < count; i++) {
    token_type_t type = keywords[i].type;
    if (!BST_insert(keyword_tree, (char *)keywords[i].str, &type,
                    sizeof(token_type_t)))
      lexer_error(ERR_INTERNAL, "inserting keyword into tree failed");
  }
}

token_t *lexer_parse(void) {
  lexer_source = read_stdin();
  char *source = lexer_source;

  list_head = calloc(1, sizeof(token_t));
  if (list_head == NULL)
    lexer_error(ERR_INTERNAL, "memory allocation failed");
  list = list_head;

  keyword_tree = BST_new();
  fill_keyword_tree(&keyword_tree);

  size_t indent_capacity = 16;
  indent_stack = malloc(indent_capacity * sizeof(size_t));
  if (indent_stack == NULL)
    lexer_error(ERR_INTERNAL, "memory allocation failed");
  indent_stack[0] = 0;
  size_t indent_top = 0;

  size_t bracket_depth = 0;
  bool at_line_start = true;

  size_t length = strlen(source);
  for (size_t i = 0; i < length; i++) {

    // Indentation at the start of a logical line
    if (at_line_start && bracket_depth == 0) {
      // count spaces, first = first character that is not a space
      size_t spaces = 0;
      size_t first = i;
      while (first < length && source[first] == ' ') {
        first++;
        spaces++;
      }

      // if there is only space, tab or comment → line is empty
      size_t end = first;
      while (end < length) {
        if (source[end] == ' ' || source[end] == '\t' || source[end] == '\r') {
          end++;
        } else if (end + 1 < length && source[end] == '/' &&
                   source[end + 1] == '*') {
          bool new_line;
          end = skip_block_comment(source, length, end, &new_line);
        } else {
          break;
        }
      }

      bool blank =
          end >= length || source[end] == '\n' ||
          (end + 1 < length && source[end] == '/' && source[end + 1] == '/');

      // empty line can not change indentation
      if (blank) {
        while (end < length && source[end] != '\n')
          end++;
        i = end; // at the \n, the loop does i++ so we skip it
        continue;
      }

      // tab can not be at start of a line
      if (source[first] == '\t')
        lexer_error(ERR_LEXICAL, "tab in indentation");

      at_line_start = false;
      i = first;

      // more spaces than before → new block
      if (spaces > indent_stack[indent_top]) {
        if (indent_top + 1 >= indent_capacity) {
          indent_capacity *= 2;
          size_t *tmp = realloc(indent_stack, indent_capacity * sizeof(size_t));
          if (tmp == NULL)
            lexer_error(ERR_INTERNAL, "memory allocation failed");
          indent_stack = tmp;
        }
        indent_stack[++indent_top] = spaces;
        lexer_push(INDENT, "");
      }
      // less spaces → block ends, one DEDENT for every level we leave
      else if (spaces < indent_stack[indent_top]) {
        while (indent_top > 0 && indent_stack[indent_top] > spaces) {
          indent_top--;
          lexer_push(DEDENT, "");
        }

        // we have to stop exactly on some older level else it is error
        if (indent_stack[indent_top] != spaces)
          lexer_error(ERR_SYNTAX, "inconsistent indentation");
      }
    }

    char current = source[i];
    char next = (i + 1 < length) ? source[i + 1] : '\0';

    // Single-char tokens
    switch (current) {
    case ',': {
      // end comma = comma + new line + ')' -> we do not make token for it
      size_t j = i + 1;
      bool new_line_after = false;
      while (j < length) {
        if (source[j] == '\n') {
          new_line_after = true;
          j++;
        } else if (source[j] == ' ' || source[j] == '\t' || source[j] == '\r') {
          j++;
        } else if (j + 1 < length && source[j] == '/' && source[j + 1] == '/') {
          while (j < length && source[j] != '\n')
            j++;
        } else if (j + 1 < length && source[j] == '/' && source[j + 1] == '*') {
          bool comment_new_line;
          j = skip_block_comment(source, length, j, &comment_new_line);
          if (comment_new_line)
            new_line_after = true;
        } else {
          break;
        }
      }

      // it has to be inside of brackets and after some item (not after '(' or
      // another comma)
      if (bracket_depth > 0 && new_line_after && j < length &&
          source[j] == ')' && list != list_head && list->type != OPEN_BRACKET &&
          list->type != COMMA)
        continue;

      lexer_push(COMMA, ",");
    }
      continue;

    case '@':
      // "@" can be only in "@main" and nothing can be after it
      if (i + 4 < length && strncmp(source + i + 1, "main", 4) == 0 &&
          !(i + 5 < length &&
            (is_alpha(source[i + 5]) || is_digit(source[i + 5]) ||
             source[i + 5] == '_'))) {
        lexer_push(AT_MAIN, "@main");
        i += 4; // skip the word main
      } else {
        lexer_error(ERR_SYNTAX, "unexpected character '@'");
      }
      continue;

    case ':':
      lexer_push(COLON, ":");
      continue;

    case ';':
      lexer_push(SEMICOLON, ";");
      continue;

    // spaces inside of line does not have a meaning
    case ' ':
    case '\t':
    case '\r':
      continue;

    // we count brackets, inside of them new lines and indentation are ignored
    case ')':
      if (bracket_depth > 0)
        bracket_depth--;
      lexer_push(CLOSE_BRACKET, ")");
      continue;

    case '(':
      bracket_depth++;
      lexer_push(OPEN_BRACKET, "(");
      continue;

    case '\n':
      // new line ends the logical line, but empty line does not give NEW_LINE
      if (bracket_depth == 0) {
        at_line_start = true;
        if (line_has_token())
          lexer_push(NEW_LINE, "\n");
      } else {
        // inside brackets the next line has to be indented more than the start
        // of the logical line
        size_t j = i + 1;
        size_t spaces = 0;
        while (j < length && source[j] == ' ') {
          j++;
          spaces++;
        }

        if (j < length && source[j] != '\n' && source[j] != '\r' &&
            source[j] != '/' && source[j] != ')' &&
            spaces <= indent_stack[indent_top])
          lexer_error(ERR_SYNTAX, "line inside brackets must be indented more");
        // line with only ')' can be aligned with the start, because of the end
        // comma
      }
      continue;

    case '+':
      lexer_push(OP_PLUS, "+");
      continue;

    case '-':
      lexer_push(OP_MINUS, "-");
      continue;

    case '*':
      lexer_push(OP_MULT, "*");
      continue;
    }

    // Operators

    if (current == '=') {
      if (next == '=') {
        lexer_push(OP_EQ, "==");
        i++;
      } else
        lexer_push(ASSIGNMENT, "=");
    } else if (current == '!') {
      if (next == '=') {
        lexer_push(OP_NOT_EQ, "!=");
        i++;
      } else
        lexer_error(ERR_LEXICAL, "unexpected character '!'");
    } else if (current == '<') {
      if (next == '=') {
        lexer_push(OP_LESS_EQ, "<=");
        i++;
      } else
        lexer_push(OP_LESS, "<");
    } else if (current == '>') {
      if (next == '=') {
        lexer_push(OP_GREATER_EQ, ">=");
        i++;
      } else
        lexer_push(OP_GREATER, ">");
    }

    // Comments

    else if (current == '/') {
      if (next == '/') {
        i++;
        while (i < length && source[i] != '\n') {
          i++;
        }

        if (bracket_depth == 0) {
          at_line_start = true;
          if (line_has_token())
            lexer_push(NEW_LINE, "\n");
        } else {
          i--; // leave \n for the switch above, it checks the indentation
        }
      }

      // Nested comments

      else if (next == '*') {
        bool new_line = false;
        i = skip_block_comment(source, length, i, &new_line);

        size_t after = i;
        while (after < length &&
               (source[after] == ' ' || source[after] == '\t' ||
                source[after] == '\r'))
          after++;

        // after multi-line comment there can be only spaces until the end of
        // line
        if (new_line && bracket_depth == 0 && after < length &&
            source[after] != '\n')
          lexer_error(ERR_LEXICAL, "token after multi-line block comment");

        i--; // loop does i++ at the end

        if (new_line && bracket_depth == 0) {
          at_line_start = true;
          if (line_has_token())
            lexer_push(NEW_LINE, "\n");
        }
      } else {
        lexer_push(OP_DIV, "/");
      }
    }

    // Strings

    else if (current == '"') {
      if (i + 2 < length && next == '"' && source[i + 2] == '"') {
        // Multi-line string
        size_t string_start = i + 3;
        size_t string_end = string_start;

        while (string_end + 2 < length &&
               !(source[string_end] == '"' && source[string_end + 1] == '"' &&
                 source[string_end + 2] == '"')) {
          string_end++;
        }
        if (string_end + 2 >= length) {
          lexer_error(ERR_LEXICAL, "unterminated multi-line string");
        }

        char *value =
            copy_string(source + string_start, string_end - string_start);
        lexer_push(VAL_STRING, value);
        free(value);

        i = string_end + 2;
      }
      // OneLine string
      else {
        size_t string_start = i + 1;
        size_t string_end = string_start;

        while (string_end < length && source[string_end] != '"') {
          if (source[string_end] == '\n') {
            lexer_error(ERR_LEXICAL, "new line in one-line string");
          }
          // Escape sequencions
          if (source[string_end] == '\\') {
            if (string_end + 1 >= length ||
                decode_escape(source[string_end + 1]) < 0) {
              lexer_error(ERR_LEXICAL, "invalid escape sequence");
            }
            string_end += 2;
          } else {
            string_end++;
          }
        }
        if (string_end >= length) {
          lexer_error(ERR_LEXICAL, "unterminated string");
        }

        char *value = malloc(string_end - string_start + 1);
        if (value == NULL) {
          lexer_error(ERR_INTERNAL, "memory allocation failed");
        }

        size_t value_length = 0;
        for (size_t k = string_start; k < string_end; k++) {
          if (source[k] == '\\') {
            k++;
            value[value_length++] = (char)decode_escape(source[k]);
          } else {
            value[value_length++] = source[k];
          }
        }
        value[value_length] = '\0';

        lexer_push(VAL_STRING, value);
        free(value);

        i = string_end;
      }
    }

    // Numbers

    else if (is_digit(current) || (current == '.' && is_digit(next))) {
      size_t number_start = i;
      token_type_t type = VAL_INT;

      // zero can not be followed by another digit
      if (current == '0') {
        i++;
        if (i < length && (is_digit(source[i]) || source[i] == '_'))
          lexer_error(ERR_LEXICAL, "integer literal must not start with zero");
      }
      // number can start with '.' (for example .5) → then there is no integer
      // part
      else if (current != '.') {
        i = skip_digits(source, length, i);
      }

      // decimal part, dot needs a digit after it
      if (i + 1 < length && source[i] == '.' && is_digit(source[i + 1])) {
        type = VAL_DOUBLE;
        i = skip_digits(source, length, i + 1);
      }

      // exponent with optional sign, digit is needed
      if (i < length && (source[i] == 'e' || source[i] == 'E')) {
        size_t exponent = i + 1;
        if (exponent < length &&
            (source[exponent] == '+' || source[exponent] == '-'))
          exponent++;

        type = VAL_DOUBLE;
        i = skip_digits(source, length, exponent);
      }

      size_t number_end = i;

      // d/D is ignored
      if (i < length && (source[i] == 'd' || source[i] == 'D')) {
        type = VAL_DOUBLE;
        i++;
      }

      char *value = malloc(number_end - number_start + 1);
      if (value == NULL)
        lexer_error(ERR_INTERNAL, "memory allocation failed");

      // copy number without underscores
      size_t value_length = 0;
      for (size_t k = number_start; k < number_end; k++) {
        if (source[k] != '_')
          value[value_length++] = source[k];
      }
      value[value_length] = '\0';

      lexer_push(type, value);
      free(value);

      i--;
    }

    else if (current == '.') {
      lexer_push(DOT, ".");
    }

    // Identifiers and keywords

    else if (current == '_' || is_alpha(current)) {
      size_t identifier_start = i;
      while (i + 1 < length &&
             (source[i + 1] == '_' || is_alpha(source[i + 1]) ||
              is_digit(source[i + 1]))) {
        i++;
      }
      size_t identifier_end = i;
      size_t identifier_length = identifier_end - identifier_start + 1;
      char *value = copy_string(source + identifier_start, identifier_length);
      if (identifier_length == 1 && value[0] == '_') {
        free(value);
        lexer_error(ERR_LEXICAL, "single underscore is not an identifier");
      }

      // checking for keywords
      token_type_t type = IDENTIFIER;
      token_type_t *keyword = BST_get_data(keyword_tree, value);
      if (keyword != NULL)
        type = *keyword;

      lexer_push(type, value);
      free(value);
    } else {
      lexer_error(ERR_LEXICAL, "unknown character");
    }
  }

  // the last line may not end with \n, so we add NEW_LINE here
  if (line_has_token())
    lexer_push(NEW_LINE, "\n");

  // close all blocks that are still open
  while (indent_top > 0) {
    indent_top--;
    lexer_push(DEDENT, "");
  }

  lexer_push(END_OF_FILE, "");

  BST_destroy_tree(keyword_tree);
  keyword_tree = NULL;

  free(indent_stack);
  indent_stack = NULL;

  free(lexer_source);
  lexer_source = NULL;

  token_t *first = list_head->next;
  free(list_head);
  list_head = NULL;
  list = NULL;

  next_token = first;
  return first;
}
