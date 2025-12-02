#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scanner.h"

#define BUFFER_INIT_SIZE 512

// Dynamic buffer for strings
typedef struct buffer {
    char *data;
    size_t length;
    size_t capacity;
} Buffer;

int buffer_init(Buffer *buffer) {
    buffer->data = malloc(BUFFER_INIT_SIZE);
    if (buffer->data == NULL) {
        return 0;
    }
    buffer->length = 0;
    buffer->capacity = BUFFER_INIT_SIZE;
    return 1;
}

int buffer_update(Buffer *buffer, char chr) {
    if (buffer->length + 1 >= buffer->capacity) {
        buffer->capacity *= 2;
        char *temp = buffer->data;
        buffer->data = realloc(buffer->data, buffer->capacity);
        if (buffer->data == NULL) {
            free(temp);
            return 0;
        }
    }
    buffer->data[buffer->length++] = chr;
    return 1;
}


// keyword or identifier check
TokenType keyword_id(const char *str) {
    char *keywords[] = {"class", "if", "else", "is", "null", "return", "var", "while", "Ifj", "static", "import", "Num", "String", "Null"};
    TokenType types[] = {TOK_CLASS, TOK_IF, TOK_ELSE, TOK_IS, TOK_NULL, TOK_RETURN, TOK_VAR, TOK_WHILE, TOK_IFJ, TOK_STATIC, TOK_IMPORT, TOK_TYPE_NUM, TOK_TYPE_STRING, TOK_TYPE_NULL};

    for (int i = 0; i < 14; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return types[i];
        }
    }
    return TOK_ID;
}

int get_next_token(Token **token) {
    FSMState state = STATE_START;
    int chr = 0;
    Buffer buffer;
    if (!buffer_init(&buffer)) {
        return 99;
    }
    
    while (chr != EOF) {
        chr = getchar();
        
        switch (state) {
            case STATE_START:
                // Ignore whitespace
                if (isblank(chr)) {
                    break;
                }
                
                // Identifiers and keywords
                if (isalpha(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    state = STATE_ID_KEYWORD;
                    break;
                }
                
                // Numbers
                if (isdigit(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    state = STATE_INT;
                    break;
                }
                
                // String literals
                if (chr == '"') {
                    state = STATE_1QUOTE;
                    break;
                }
                
                // Slash - comment or division
                if (chr == '/') {
                    state = STATE_SLASH;
                    break;
                }
                
                if (chr == '<') {
                    state = STATE_LT;
                    break;
                }
                if (chr == '>') {
                    state = STATE_GT;
                    break;
                }
                if (chr == '=') {
                    state = STATE_ASSIGN;
                    break;
                }
                if (chr == '!') {
                    state = STATE_NEQ;
                    break;
                }

                // Single character tokens
                switch (chr)
                {
                    case '\n':
                        (*token)->type = TOK_EOL;
                        return 0;
                    case EOF:
                        (*token)->type = TOK_EOF;
                        return 0;
                    case '+':
                        (*token)->type = TOK_PLUS;
                        return 0;
                    case '-':
                        (*token)->type = TOK_MINUS;
                        return 0;
                    case '*':
                        (*token)->type = TOK_ASTERISK;
                        return 0;
                    case '{':
                        (*token)->type = TOK_LBRACE;
                        return 0;
                    case '}':
                        (*token)->type = TOK_RBRACE;
                        return 0;
                    case '(':
                        (*token)->type = TOK_LPAR;
                        return 0;
                    case ')':
                        (*token)->type = TOK_RPAR;
                        return 0;
                    case '.':
                        (*token)->type = TOK_DOT;
                        return 0;
                    case ',':
                        (*token)->type = TOK_COMMA;
                        return 0;
                    default:
                        return 1;
                }

                break;
                
            case STATE_ID_KEYWORD:
                if (isalpha(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                if (isdigit(chr) || chr == '_') {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    state = STATE_ID;
                }
                else {
                    ungetc(chr, stdin);
                    if (!buffer_update(&buffer, '\0')) {
                        return 99;
                    }
                    
                    (*token)->type = keyword_id(buffer.data);
                    if ((*token)->type != TOK_ID) {
                        return 0;
                    }

                    (*token)->data.str_value = malloc(strlen(buffer.data) + 1);
                    if ((*token)->data.str_value == NULL) {
                        return 99;
                    }
                    strcpy((*token)->data.str_value, buffer.data);
                    return 0;
                }
                break;
            case STATE_ID:
                if (isalnum(chr) || chr == '_') {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                }
                else {
                    ungetc(chr, stdin);
                    if (!buffer_update(&buffer, '\0')) {
                        return 99;
                    }
                    
                    (*token)->type = TOK_ID;
                    (*token)->data.str_value = malloc(strlen(buffer.data) + 1);
                    if ((*token)->data.str_value == NULL) {
                        return 99;
                    }
                    strcpy((*token)->data.str_value, buffer.data);
                    return 0;
                }
                break;

            case STATE_LT:
                if (chr == '=') {
                    (*token)->type = TOK_LTE;
                    return 0;
                }
                else {
                    ungetc(chr, stdin);
                    (*token)->type = TOK_LT;
                    return 0;
                }
                break;

            case STATE_GT:
                if (chr == '=') {
                    (*token)->type = TOK_GTE;
                    return 0;
                }
                else {
                    ungetc(chr, stdin);
                    (*token)->type = TOK_GT;
                    return 0;
                }
                break;
                
            case STATE_INT:
                if (isdigit(chr)) {
                    if (buf_idx < 255) buffer[buf_idx++] = chr;
                } else if (chr == '.' && isdigit(getchar())) {
                    ungetc(chr, stdin);
                    buffer[buf_idx++] = chr;
                    state = STATE_FLOAT;
                } else {
                    ungetc(chr, stdin);
                    buffer[buf_idx] = '\0';
                    token.type = TOK_CONST_INT;
                    token.data.num_int_value = strtoll(buffer, NULL, 10);
                    return token;
                }
                break;
                
            case STATE_FLOAT:
                if (isdigit(chr)) {
                    if (buf_idx < 255) buffer[buf_idx++] = chr;
                } else {
                    ungetc(chr, stdin);
                    buffer[buf_idx] = '\0';
                    token.type = TOK_CONST_FLOAT;
                    token.data.num_float_value = strtod(buffer, NULL);
                    return token;
                }
                break;
                
            case STATE_STRING:
                break;
                
            case STATE_ML_STRING:
                break;
        }
    }
    (*token)->type = TOK_EOF;
    return 0;
}