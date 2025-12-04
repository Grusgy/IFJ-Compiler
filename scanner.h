/*
    Implementace překladače imperativního jazyka IFJ25

    Petr Parkan:    xparkap00
*/

#ifndef SCANNER_H
#define SCANNER_H

// Token types                  // inspirace podle demonstračního cvičení k projektu - Ondřej Ondryáš
typedef enum token_type {
    // Prolog
    TOK_PROLOG,

    // Keywords
    TOK_CLASS,
    TOK_IF,
    TOK_ELSE,
    TOK_IS,
    TOK_NULL,
    TOK_RETURN,
    TOK_VAR,
    TOK_WHILE,
    TOK_IFJ,
    TOK_STATIC,
    TOK_IMPORT,
    TOK_FOR,
    TOK_TYPE_NUM,
    TOK_TYPE_STRING,
    TOK_TYPE_NULL,

    // Built-in functions
    TOK_FUNC_READ_STR,
    TOK_FUNC_READ_NUM,
    TOK_FUNC_WRITE,
    TOK_FUNC_FLOOR,
    TOK_FUNC_STR,
    TOK_FUNC_LENGTH,
    TOK_FUNC_SUBSTRING,
    TOK_FUNC_STRCMP,
    TOK_FUNC_ORD,
    TOK_FUNC_CHR,

    // Identifiers
    TOK_ID,
    TOK_GLOBAL_ID,

    // Constants
    TOK_CONST_INT,
    TOK_CONST_FLOAT,
    TOK_CONST_STR,
    TOK_CONST_ML_STR,

    // Operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_ASTERISK,
    TOK_DIV,
    TOK_LT,
    TOK_GT,
    TOK_LTE,
    TOK_GTE,
    TOK_ASSIGN,
    TOK_EQ,
    TOK_NEQ,

    // Misc
    TOK_EOL,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAR,
    TOK_RPAR,
    TOK_COMMA,
    TOK_EOF
} TokenType;

// Token structure              // podle demonstračního cvičení k projektu - Ondřej Ondryáš
typedef union token_data {
    long long int num_int_value;
    double num_float_value;
    char *str_value;
} TokenData;

typedef struct token {
    TokenType type;
    TokenData data;
} Token;

// Function for reading a token from input
int get_next_token(Token *token);

#endif
