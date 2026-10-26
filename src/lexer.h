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
    char * value;
    token_type_t * next;
}token_t;

token_t *lexer_parse(char *source);
void lexer_push(token_type_t type, char *value);
