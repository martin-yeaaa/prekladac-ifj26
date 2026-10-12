typedef enum token_type{
    PLUS,
    MINUS,
    INTEGER
    // TODO doplnit vsechny typy tokenu
} token_type_t;

typedef struct token
{
    token_type_t t;
    char * value;
    // odkaz na dalsi token
}token_t;