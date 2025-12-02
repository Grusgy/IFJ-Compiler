#ifndef SCANNER_H
#define SCANNER_H

#include <string.h>

// Token types
typedef enum token_type {
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
    TOK_TYPE_NUM,
    TOK_TYPE_STRING,
    TOK_TYPE_NULL,

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
    TOK_EQ,
    TOK_NEQ,

    // Misc
    TOK_EOL,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAR,
    TOK_RPAR,
    TOK_ASSIGN,
    TOK_DOT,
    TOK_COMMA,
    TOK_EOF
} TokenType;

typedef union token_data {
    long long int num_int_value;
    double num_float_value;
    char *str_value;
} TokenData;

typedef struct token {
    TokenType type;
    TokenData data;
} Token;

int get_next_token(Token **token);

// FSM states
typedef enum {
    STATE_START,
    STATE_ID_KEYWORD,
    STATE_ZERO,
    STATE_INT,
    STATE_FLOAT,

    STATE_QUOTE1,
    STATE_STRING,
    STATE_ESCAPE,

    STATE_QUOTE2,
    STATE_ML_STRING,
    STATE_ENDQUOTE1,
    STATE_ENDQUOTE2,

    STATE_LT,
    STATE_GT,
    STATE_ASSIGN,
    STATE_NEQ,
    STATE_SLASH,
    STATE_LINE_COMMENT,
    STATE_BLOCK_COMMENT,
    STATE_ONECHAR_TOK
} FSMState;

#endif