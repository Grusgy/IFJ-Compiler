#ifndef SCANNER_H
#define SCANNER_H

#include <string.h>

#define FOREACH_TOKENTYPE(TYPE) \
        TYPE(TOK_CLASS) \
        TYPE(TOK_IF) \
        TYPE(TOK_ELSE) \
        TYPE(TOK_IS) \
        TYPE(TOK_NULL) \
        TYPE(TOK_RETURN) \
        TYPE(TOK_VAR) \
        TYPE(TOK_WHILE) \
        TYPE(TOK_IFJ) \
        TYPE(TOK_STATIC) \
        TYPE(TOK_IMPORT) \
        TYPE(TOK_TYPE_NUM) \
        TYPE(TOK_TYPE_STRING) \
        TYPE(TOK_TYPE_NULL) \
        TYPE(TOK_ID) \
        TYPE(TOK_GLOBAL_ID) \
        TYPE(TOK_CONST_INT) \
        TYPE(TOK_CONST_FLOAT) \
        TYPE(TOK_CONST_STR) \
        TYPE(TOK_CONST_ML_STR) \
        TYPE(TOK_PLUS) \
        TYPE(TOK_MINUS) \
        TYPE(TOK_ASTERISK) \
        TYPE(TOK_DIV) \
        TYPE(TOK_LT) \
        TYPE(TOK_GT) \
        TYPE(TOK_LTE) \
        TYPE(TOK_GTE) \
        TYPE(TOK_EQ) \
        TYPE(TOK_NEQ) \
        TYPE(TOK_EOL) \
        TYPE(TOK_LBRACE) \
        TYPE(TOK_RBRACE) \
        TYPE(TOK_LPAR) \
        TYPE(TOK_RPAR) \
        TYPE(TOK_ASSIGN) \
        TYPE(TOK_DOT) \
        TYPE(TOK_COMMA) \
        TYPE(TOK_EOF)

#define GENERATE_STRING(STRING) #STRING,

static const char *TOKENTYPE_STRING[] = {
    FOREACH_TOKENTYPE(GENERATE_STRING)
};


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
    STATE_ML_START_WS,
    STATE_ML_END_WS,

    STATE_SLASH,
    STATE_LINE_COMMENT,
    STATE_BLOCK_COMMENT,
    STATE_ONECHAR_TOK
} FSMState;

#endif