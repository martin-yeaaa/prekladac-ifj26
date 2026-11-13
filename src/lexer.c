// Project: prekladac-ifj26
// File: lexer.c
// Author: Michal Bína (xbinami00), Jiří Švec (xsvecji00)

// Implementation of lexer


#include "lexer.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bst.h"


static token_t *list;
static token_t *list_head;
static token_t *next_token;
static BSTNode keyword_tree;

// TODO create token returning function

void lexer_push(token_type_t type, char *value)
{
    list->next =(token_t *) malloc(sizeof(token_t));
    list->next->type = type;
    list->next->value = value;

    list = list->next;
}


static void lexer_error(int code, const char *message)
{
    fprintf(stderr, "Lexer error: %s\n", message);
    
    // TODO clean allocated memory

    exit(code);
}

static char *copy_string(const char *src, size_t length)
{
    char *copy = malloc(length + 1);
    if (copy == NULL)
        lexer_error(99, "memory allocation failed");

    memcpy(copy, src, length);
    copy[length] = '\0';
    return copy;
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}


static int decode_escape(char c)
{
    switch (c) {
        case '\\': return '\\';
        case '"':  return '"';
        case 'n':  return '\n';
        case 'r':  return '\r';
        case 't':  return '\t';
        default:   return -1;
    }
}

static size_t skip_digits(const char *source, size_t length, size_t i)
{
    if (i >= length || !is_digit(source[i]))
        lexer_error(1, "digit expected in number literal");

    while (i < length && (is_digit(source[i]) || source[i] == '_'))
        i++;

    if (source[i - 1] == '_')
        lexer_error(1, "underscore must be between two digits");

    return i;
}

void fill_keyword_tree(BSTNode *keyword_tree)
{
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
    } keywords[] = {
        { "def",     KW_DEF },
        { "do",      KW_DO },
        { "Double",  KW_DOUBLE },
        { "else",    KW_ELSE },
        { "if",      KW_IF },
        { "import",  KW_IMPORT },
        { "Int",     KW_INT },
        { "return",  KW_RETURN },
        { "String",  KW_STRING },
        { "then",    KW_THEN },
        { "Unit",    KW_UNIT },
        { "val",     KW_VAL },
        { "var",     KW_VAR },
        { "while",   KW_WHILE },

        { "Boolean",   KW_BOOLEAN },
        { "case",      KW_CASE },
        { "class",     KW_CLASS },
        { "extension", KW_EXTENSION },
        { "for",       KW_FOR },
        { "List",      KW_LIST },
        { "match",     KW_MATCH },
        { "type",      KW_TYPE },

        { "true",      VAL_BOOL },
        { "false",     VAL_BOOL }
    };

    size_t count = sizeof(keywords) / sizeof(keywords[0]);

    for (size_t i = 0; i < count; i++) {
        token_type_t type = keywords[i].type;
        if (!BST_insert(keyword_tree, (char *)keywords[i].str, &type, sizeof(token_type_t)))
            lexer_error(99, "inserting keyword into tree failed");
    }
}

token_t *lexer_parse(char *source)
{
    list_head = calloc(1, sizeof(token_t));
    if (list_head == NULL)
        lexer_error(99, "memory allocation failed");
    list = list_head;

    keyword_tree = BST_new();
    fill_keyword_tree(&keyword_tree);

    size_t length = strlen(source);
    for (size_t i = 0; i < length; i++) {
        char current = source[i];
        char next = (i + 1 < length) ? source[i + 1] : '\0';

        // Single-char tokens
        switch (current) {
            case ',':
            lexer_push(COMMA, ",");
            continue;

            case '@':
            lexer_push(AT, "@");
            continue;

            case ':':
            lexer_push(COLON, ":");
            continue;

            case ';':
            lexer_push(SEMICOLON, ";");
            continue;

            case ' ':
            lexer_push(SPACE, " ");
            continue;

            case ')':
            lexer_push(CLOSE_BRACKET, ")");
            continue;

            case '(':
            lexer_push(OPEN_BRACKET, "(");
            continue;

            case '\n':
            lexer_push(NEW_LINE, "\n");
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

        if (current == '='){
            if (next == '='){
                lexer_push(OP_EQ, "==");
                i++;
            }
            else
                lexer_push(ASSIGNMENT, "=");
        }
        else if (current == '!'){
            if (next == '='){
                lexer_push(OP_NOT_EQ, "!=");
                i++;
            }
            else
                lexer_error(1, "unexpected character '!'");
        }
        else if (current == '<'){
            if (next == '='){
                lexer_push(OP_LESS_EQ, "<=");
                i++;
            }
            else
                lexer_push(OP_LESS, "<");
        }
        else if (current == '>'){
            if (next == '='){
                lexer_push(OP_GREATER_EQ, ">=");
                i++;
            }
            else
                lexer_push(OP_GREATER, ">");
        }

        // Comments

        else if (current == '/'){
            if (next == '/'){
                i++;
                while (i < length && source[i] != '\n'){
                    i++;
                }
                lexer_push(NEW_LINE, "\n");
            }

            // Nested comments

            else if (next == '*') {
                i += 2;
                size_t depth = 1;
                bool new_line = false;

                while (i < length && depth > 0) {
                    if (source[i] == '\n') {
                        new_line = true;
                        i++;
                    }
                    else if (i + 1 < length && source[i] == '/' && source[i + 1] == '*') {
                        depth++;
                        i += 2;
                    }
                    else if (i + 1 < length && source[i] == '*' && source[i + 1] == '/') {
                        depth--;
                        i += 2;
                    }
                    else {
                        i++;
                    }
                }

                if (depth != 0)
                    lexer_error(1, "unterminated block comment");

                size_t after = i;
                while (after < length && source[after] == ' ')
                    after++;

                if (new_line && after < length && source[after] != '\n')
                    lexer_error(1, "token after multi-line block comment");

                i--;

                if (new_line)
                    lexer_push(NEW_LINE, "\n");
                else
                    lexer_push(SPACE, " ");
            }
            else {
                lexer_push(OP_DIV, "/");
            }
        }

        // Strings

        else if (current == '"') {
            if (i + 2 < length && next == '"' && source[i+2] == '"') {
                // Multi-line string
                size_t string_start = i + 3;
                size_t string_end = string_start;

                while (string_end + 2 < length && !(source[string_end] == '"' && source[string_end+1] == '"' && source[string_end+2] == '"')){
                    string_end++;
                }
                if (string_end + 2 >= length) {
                    lexer_error(1, "unterminated multi-line string");
                }

                char *value = copy_string(source + string_start, string_end - string_start);
                lexer_push(VAL_STRING, value);
                free(value);

                i = string_end + 2;
            }
            // OneLine string
            else {
                size_t string_start = i + 1;
                size_t string_end = string_start;

                while (string_end < length && source[string_end] != '"'){
                    if (source[string_end] == '\n') {
                        lexer_error(1, "new line in one-line string");
                    }
                    // Escape sequencions
                    if (source[string_end] == '\\') {
                        if (string_end + 1 >= length || decode_escape(source[string_end+1]) < 0) {
                            lexer_error(1, "invalid escape sequence");
                        }
                        string_end += 2;
                    }
                    else {
                        string_end++;
                    }
                }
                if (string_end >= length) {
                    lexer_error(1, "unterminated string");
                }

                char *value = malloc(string_end - string_start + 1);
                if (value == NULL) {
                    lexer_error(99, "memory allocation failed");
                }

                size_t value_length = 0;
                for (size_t k = string_start; k < string_end; k++) {
                    if (source[k] == '\\') {
                        k++;
                        value[value_length++] = (char) decode_escape(source[k]);
                    }
                    else {
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

            if (current == '0') {
                i++;
                if (i < length && (is_digit(source[i]) || source[i] == '_'))
                    lexer_error(1, "integer literal must not start with zero");
            }
            else if (current != '.') {
                i = skip_digits(source, length, i);
            }

            if (i + 1 < length && source[i] == '.' && is_digit(source[i+1])) {
                type = VAL_DOUBLE;
                i = skip_digits(source, length, i + 1);
            }

            if (i < length && (source[i] == 'e' || source[i] == 'E')) {
                size_t exponent = i + 1;
                if (exponent < length && (source[exponent] == '+' || source[exponent] == '-'))
                    exponent++;

                type = VAL_DOUBLE;
                i = skip_digits(source, length, exponent);
            }

            size_t number_end = i;

            if (i < length && (source[i] == 'd' || source[i] == 'D')) {
                type = VAL_DOUBLE;
                i++;
            }

            char *value = malloc(number_end - number_start + 1);
            if (value == NULL)
                lexer_error(99, "memory allocation failed");

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

        // Identifiers and keywords

        else if (current == '_' || is_alpha(current)) {
            size_t identifier_start = i;
            while (i+1 < length && (source[i+1] == '_' || is_alpha(source[i+1]) || is_digit(source[i+1]))){
                i++;
            }
            size_t identifier_end = i;
            size_t identifier_length = identifier_end - identifier_start + 1;
            char *value = copy_string(source + identifier_start, identifier_length);
            if (identifier_length == 1 && value[0] == '_'){
                free(value);
                lexer_error(1, "single underscore is not an identifier");
            }

            // checking for keywords
            token_type_t type = IDENTIFIER;
            token_type_t *keyword = BST_get_data(keyword_tree, value);
            if (keyword != NULL)
                type = *keyword;

            lexer_push(type, value);
            free(value);
        }
        else {
            lexer_error(1, "unknown character");
        }
    }

    lexer_push(END_OF_FILE, "");

    return // TODO free allocated memory
}