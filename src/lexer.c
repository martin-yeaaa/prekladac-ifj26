#include <lexer.h>      // lexer
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>


static token_t *list;

void lexer_push(token_type_t type, char *value)
{
    list->next =(token_t *) malloc(sizeof(token_t));
    list->next->type = type;
    list->next->value = value;

    list = list->next;
}

token_t *lexer_parse(char *source)
{
    list = (token_t *) malloc(sizeof(token_t));
    bzero(list, sizeof(token_t));

    size_t length = strlen(source);
    for (size_t i=0; i < length; i++) {
        char current = source[i];

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
    }
}