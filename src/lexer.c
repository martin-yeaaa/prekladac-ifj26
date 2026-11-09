#include <lexer.h>      // lexer
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <bst.h>


static token_t *list;

// TODO create token returning function

void lexer_push(token_type_t type, char *value)
{
    list->next =(token_t *) malloc(sizeof(token_t));
    list->next->type = type;
    list->next->value = value;

    list = list->next;
}

void fill_keyword_tree(BSTNode *keyword_tree)
{
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
        { "type",      KW_TYPE }
    };

    size_t count = sizeof(keywords) / sizeof(keywords[0]);

    for (size_t i = 0; i < count; i++) {
        token_type_t type = keywords[i].type;
        BST_insert(keyword_tree, (char *)keywords[i].str, &type, sizeof(token_type_t));
    }
}

token_t *lexer_parse(char *source)
{
    list = (token_t *) malloc(sizeof(token_t));
    bzero(list, sizeof(token_t));

    size_t length = strlen(source);
    for (size_t i=0; i < length; i++) {
        char current = source[i];

        // Single-char tokens
        switch (current) {
            case ',':
            lexer_push(COMMA, ",");
            break;
            
            case '@':
            lexer_push(AT, "@");
            break;
            
            case ':':
            lexer_push(COLON, ":");
            break;

            case ';':
            lexer_push(SEMICOLON, ";");
            break;
            
            case ' ':
            lexer_push(SPACE, " ");
            break;
            
            case ')':
            lexer_push(CLOSE_BRACKET, ")");
            break;

            case '(':
            lexer_push(OPEN_BRACKET, "(");
            break;

            case '\0':
            lexer_push(END_OF_FILE, "\0");
            break;

            case '\n':
            lexer_push(NEW_LINE, "\n");
            break;

            case '+':
            lexer_push(OP_PLUS, "+");
            break;

            case '*':
            lexer_push(OP_MULT, "*");
            break;
        }


        // Operators

        if (current == '='){
            if (i + 1 < length && source[i+1] == '='){
                lexer_push(OP_EQ, "==");
                i++;
            }   
            else
                lexer_push(ASSIGNMENT, "=");
        }
        else if (current == '!'){
            if (i + 1 < length && source[i+1] == '='){
                lexer_push(OP_NOT_EQ, "!=");
                i++;
            }   
            else
                exit(1);
        }
        else if (current == '<'){
            if (i + 1 < length && source[i+1] == '='){
                lexer_push(OP_LESS_EQ, "<=");
                i++;
            }   
            else
                lexer_push(OP_LESS, "<");
        }
        else if (current == '>'){
            if (i + 1 < length && source[i+1] == '='){
                lexer_push(OP_GREATER_EQ, ">=");
                i++;
            }   
            else
                lexer_push(OP_GREATER, ">");
        }
        
        // Comments
        
        else if (current == '/'){
            if (i + 1 < length && source[i+1] == '/'){
                i++;
                while (i < length && source[i] != '\n'){
                    i++;
                }
                lexer_push(NEW_LINE, "\n");
            }   

            // Nested comments

            else if (i + 1 < length && source[i + 1] == '*') {
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
                    exit(1);  // Unclosed comment

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

        // 
        else if (current == '"') {
            if (i + 1 < length && source[i+1] == '"'){
                if (i + 2 < length){
                    if (source[i+2] == '"') {
                        // Multi-line string
                        int string_start = i; // with "
                while (i+1 < length && source[i+1] != '"'){
                    i++;
                    // Escape sequencions
                    if (source[i] == '\\')  {
                        if (i + 1 < length) {
                            if (source[i+1] == '\\' || source[i+1] == '"' || source[i+1] == 'n' || 
                                source[i+1] == 'r' || source[i+1] == 't'){
                                i+=2;
                            }
                            else {
                                exit(1);
                            }                           
                        }
                        else {
                            exit(1);
                        }
                    }
                }
                if (!(i+3 < length && source[i+2] == '"' && source[i+3] == '"')) {
                    exit(1);
                }
                int string_end = i;

                size_t string_length = string_end - (string_start+1);

                char *value = malloc(string_length + 1);

                memcpy(value, source + string_start, string_length);
                value[string_length] = '\0';

                lexer_push(VAL_STRING, value);
                    }
                    else {
                        lexer_push(VAL_STRING, "");
                        i+=2;
                    }
                } 
                else {
                    lexer_push(VAL_STRING, "");
                    i+=2;
                }
            }
            // OneLine string
            else { 
                int string_start = i; // with "
                while (i+1 < length && source[i+1] != '"'){
                    i++;
                    // Escape sequencions
                    if (source[i] == '\\')  {
                        if (i + 1 < length) {
                            if (source[i+1] == '\\' || source[i+1] == '"' || source[i+1] == 'n' || 
                                source[i+1] == 'r' || source[i+1] == 't'){
                                i+=2;
                            }
                            else {
                                exit(1);
                            }                           
                        }
                        else {
                            exit(1);
                        }
                    }
                    else {
                        if (source[i] == '\n') {
                            exit(1);
                        }
                    }
                }
                int string_end = i;

                size_t string_length = string_end - (string_start+1);

                char *value = malloc(string_length + 1);

                memcpy(value, source + string_start, string_length);
                value[string_length] = '\0';

                lexer_push(VAL_STRING, value);
            }
        }
        else if ((current == '_' || (current > 64 && current < 91) || (current > 97 && current < 123))) {
            int identifier_start = i;
            while (i+1 < length && (source[i+1] == '_' || ((source[i+1] > 64 && source[i+1] < 91) || (source[i+1] > 97 && source[i+1] < 123)))){
                i++;
            }
            int identifier_end = i;
            size_t identifier_length = identifier_end - (identifier_start+1);
            char *value = malloc(identifier_length + 1);
            memcpy(value, source + identifier_start, identifier_length);
            value[identifier_length] = '\0';
            if (identifier_lenght == 1 && value[0] == '_'){
                exit(1);
            }

            // checking for keywords
            
        }
    }
}