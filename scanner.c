#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scanner.h"

#define BUFFER_INIT_SIZE 128

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
    int ml_string_end = 0;
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
                
                //// Numbers
                //if (isdigit(chr)) {
                //    if (!buffer_update(&buffer, chr)) {
                //        return 99;
                //    }
                //    state = STATE_INT;
                //    break;
                //}
                
                // Slash - comment or division
                if (chr == '/') {
                    state = STATE_SLASH;
                    break;
                }
                
                switch (chr) {
                    // Newline and EOF
                    case '\n':
                        (*token)->type = TOK_EOL;
                        return 0;
                    case EOF:
                        (*token)->type = TOK_EOF;
                        return 0;

                    // <, <=, >, >=, =, ==, !=
                    case '<':
                        chr = getchar();
                        if (chr == '=') {
                            (*token)->type = TOK_LTE;
                        } else {
                            ungetc(chr, stdin);
                            (*token)->type = TOK_LT;
                        }
                        return 0;
                    case '>':
                        chr = getchar();
                        if (chr == '=') {
                            (*token)->type = TOK_GTE;
                        } else {
                            ungetc(chr, stdin);
                            (*token)->type = TOK_GT;
                        }
                        return 0;
                    case '=':
                        chr = getchar();
                        if (chr == '=') {
                            (*token)->type = TOK_EQ;
                        } else {
                            ungetc(chr, stdin);
                            (*token)->type = TOK_ASSIGN;
                        }
                        return 0;
                    case '!':
                        chr = getchar();
                        if (chr == '=') {
                            (*token)->type = TOK_NEQ;
                            return 0;
                        } else {
                            ungetc(chr, stdin);
                            return 1;
                        }

                    // Start of string
                    case '\"':
                        state = STATE_QUOTE1;
                        break;

                    // Single character tokens
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
                    case ',':
                        (*token)->type = TOK_COMMA;
                        return 0;
                    default:
                        return 1;
                }

                break;
                
            case STATE_ID_KEYWORD:
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
                    (*token)->type = keyword_id(buffer.data);
                    if ((*token)->type != TOK_ID) {
                        return 0;
                    }
                    (*token)->data.str_value = malloc(buffer.length);
                    if ((*token)->data.str_value == NULL) {
                        return 99;
                    }
                    strcpy((*token)->data.str_value, buffer.data);
                    return 0;
                }
                break;

            case STATE_QUOTE1:
                switch (chr) {
                    case '\n':
                    case EOF:
                        ungetc(chr, stdin);
                        return 1;
                    case '\"':
                        state = STATE_QUOTE2;
                        break;
                    case '\\':
                        state = STATE_ESCAPE;
                        break;
                    default:
                        if (!buffer_update(&buffer, chr)) {
                            return 99;
                        }
                        state = STATE_STRING;
                        break;
                }
                break;

            case STATE_STRING:
                switch (chr) {
                    case '\n':
                    case EOF:
                        ungetc(chr, stdin);
                        return 1;
                    case '\\':
                        state = STATE_ESCAPE;
                        break;
                    case '\"':
                        if (!buffer_update(&buffer, '\0')) {
                            return 99;
                        }
                        (*token)->type = TOK_CONST_STR;
                        (*token)->data.str_value = malloc(buffer.length);
                        if ((*token)->data.str_value == NULL) {
                            return 99;
                        }
                        strcpy((*token)->data.str_value, buffer.data);
                        return 0;
                    default:
                        if (!buffer_update(&buffer, chr)) {
                            return 99;
                        }
                        break;
                }
                break;

            case STATE_ESCAPE:
                switch (chr) {
                    case '\\':
                        if (!buffer_update(&buffer, '\\')) {
                            return 99;
                        }
                        state = STATE_STRING;
                        break;
                    case 'n':
                        if (!buffer_update(&buffer, '\n')) {
                            return 99;
                        }
                        state = STATE_STRING;
                        break;
                    case 'r':
                        if (!buffer_update(&buffer, '\r')) {
                            return 99;
                        }
                        state = STATE_STRING;
                        break;
                    case 't':
                        if (!buffer_update(&buffer, '\t')) {
                            return 99;
                        }
                        state = STATE_STRING;
                        break;
                    case '\"':
                        if (!buffer_update(&buffer, '\"')) {
                            return 99;
                        }
                        state = STATE_STRING;
                        break;
                    case 'x': {
                        int digit16 = getchar();
                        if (!isxdigit(digit16)) {
                            ungetc(digit16, stdin);
                            return 1;
                        }
                        int digit1 = getchar();
                        if (!isxdigit(digit1)) {
                            ungetc(digit1, stdin);
                            ungetc(digit16, stdin);
                            return 1;
                        }
                        if (isupper(digit16)) {     // ASCII value of A-F(65-70) - 55 = 10-15
                            digit16 -= 55;
                        }
                        else if (islower(digit16)) {     // ASCII value of a-f(97-102) - 87 = 10-15
                            digit16 -= 87;
                        }
                        else {                      // ASCII value of 0-9(48-57) - 48 = 0-9
                            digit16 -= 48;
                        }

                        if (isupper(digit1)) {
                            digit1 -= 55;
                        }
                        else if (islower(digit1)) {
                            digit1 -= 87;
                        }
                        else {
                            digit1 -= 48;
                        }

                        if (!buffer_update(&buffer, (char)(digit16 * 16 + digit1))) {
                            return 99;
                        }
                        state = STATE_STRING;
                        break;
                    }
                    default:
                        ungetc(chr, stdin);
                        return 1;
                }
                break;

            case STATE_QUOTE2:
                if (chr == '\"') {
                    state = STATE_ML_STRING;
                    break;
                }
                ungetc(chr, stdin);
                (*token)->type = TOK_CONST_STR;
                (*token)->data.str_value = malloc(1);
                if ((*token)->data.str_value == NULL) {
                    return 99;
                }
                (*token)->data.str_value[0] = '\0';
                return 0;
                break;

            case STATE_ML_START_WS:
                if (chr == EOF) {
                    ungetc(chr, stdin);
                    return 1;
                }
                if (isblank(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    break;
                }
                if (chr == '\n') {
                    buffer.length = 0;
                    state = STATE_ML_STRING;;
                    break;
                }
                if (chr == '\"') {
                    state = STATE_ENDQUOTE1;
                    break;
                }
                if (!buffer_update(&buffer, chr)) {
                    return 99;
                }
                state = STATE_ML_STRING;
                break;

            case STATE_ML_STRING:
                if (chr == EOF) {
                    ungetc(chr, stdin);
                    return 1;
                }
                if (chr == '\"') {
                    state = STATE_ENDQUOTE1;
                    break;
                }
                if (chr == '\n') {
                    ml_string_end = buffer.length;
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    state = STATE_ML_END_WS;
                    break;
                }
                if (!buffer_update(&buffer, chr)) {
                    return 99;
                }
                break;

            case STATE_ML_END_WS:
                if (chr == EOF) {
                    ungetc(chr, stdin);
                    return 1;
                }
                if (isblank(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    break;
                }
                if (chr == '\n') {
                    ml_string_end = buffer.length;
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    break;
                }
                if (chr == '\"') {
                    chr = getchar();
                    if (chr == '\"') {
                        chr = getchar();
                        if (chr == '\"') {
                            buffer.length = ml_string_end;
                            if (!buffer_update(&buffer, '\0')) {
                                return 99;
                            }
                            (*token)->type = TOK_CONST_ML_STR;
                            (*token)->data.str_value = malloc(buffer.length);
                            if ((*token)->data.str_value == NULL) {
                                return 99;
                            }
                            strcpy((*token)->data.str_value, buffer.data);
                            return 0;
                        }
                        ungetc(chr, stdin);
                        if (!buffer_update(&buffer, '\"')) {
                            return 99;
                        }
                        if (!buffer_update(&buffer, '\"')) {
                            return 99;
                        }
                        state = STATE_ML_STRING;;
                        break;
                    }
                    ungetc(chr, stdin);
                    if (!buffer_update(&buffer, '\"')) {
                        return 99;
                    }
                    state = STATE_ML_STRING;;
                    break;
                }
                if (!buffer_update(&buffer, chr)) {
                    return 99;
                }
                state = STATE_ML_STRING;;
                break;

            case STATE_ENDQUOTE1:
                if (chr == EOF) {
                    ungetc(chr, stdin);
                    return 1;
                }
                if (chr == '\"') {
                    state = STATE_ENDQUOTE2;;
                    break;
                }
                if (!buffer_update(&buffer, '\"')) {
                    return 99;
                }
                ungetc(chr, stdin);
                state = STATE_ML_STRING;;
                break;

            case STATE_ENDQUOTE2:
                if (chr == EOF) {
                    ungetc(chr, stdin);
                    return 1;
                }
                if (chr == '\"') {
                    if (!buffer_update(&buffer, '\0')) {
                        return 99;
                    }
                    (*token)->type = TOK_CONST_ML_STR;
                    (*token)->data.str_value = malloc(buffer.length);
                    if ((*token)->data.str_value == NULL) {
                        return 99;
                    }
                    strcpy((*token)->data.str_value, buffer.data);
                    return 0;
                }
                if (!buffer_update(&buffer, '\"')) {
                    return 99;
                }
                if (!buffer_update(&buffer, '\"')) {
                    return 99;
                }
                ungetc(chr, stdin);
                state = STATE_ML_STRING;;
                break;

            case STATE_SLASH:
                if (chr == '/') {
                    state = STATE_LINE_COMMENT;
                    break;
                } else if (chr == '*') {
                    state = STATE_BLOCK_COMMENT;
                    break;
                } else {
                    ungetc(chr, stdin);
                    (*token)->type = TOK_DIV;
                    return 0;
                }

            case STATE_LINE_COMMENT:
                if (chr == '\n' || chr == EOF) {
                    ungetc(chr, stdin);
                    state = STATE_START;
                }
                break;
            
            //case STATE_INT:
            //    if (isdigit(chr)) {
            //        if (buf_idx < 255) buffer[buf_idx++] = chr;
            //    } else if (chr == '.' && isdigit(getchar())) {
            //        ungetc(chr, stdin);
            //        buffer[buf_idx++] = chr;
            //        state = STATE_FLOAT;
            //    } else {
            //        ungetc(chr, stdin);
            //        buffer[buf_idx] = '\0';
            //        token.type = TOK_CONST_INT;
            //        token.data.num_int_value = strtoll(buffer, NULL, 10);
            //        return token;
            //    }
            //    break;
            //    
            //case STATE_FLOAT:
            //    if (isdigit(chr)) {
            //        if (buf_idx < 255) buffer[buf_idx++] = chr;
            //    } else {
            //        ungetc(chr, stdin);
            //        buffer[buf_idx] = '\0';
            //        token.type = TOK_CONST_FLOAT;
            //        token.data.num_float_value = strtod(buffer, NULL);
            //        return token;
            //    }
            //    break;
        }
    }
    (*token)->type = TOK_EOF;
    free(buffer.data);
    return 0;
}

