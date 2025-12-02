#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "scanner.h"

// FSM states
typedef enum {
    STATE_START,

    STATE_PROLOG_QUOTES,
    STATE_PROLOG_FOR,
    STATE_PROLOG_IFJ,

    STATE_ID_KEYWORD,
    STATE_GLOBAL_ID,

    STATE_BUILTIN_NAMESPACE,
    STATE_BUILTIN_DOT,
    STATE_BUILTIN_FUNC,

    STATE_DEC_INT,
    STATE_HEX_INT,
    STATE_NUM,
    STATE_FLOAT_DECIMAL,
    STATE_FLOAT_EXP_SIGN,
    STATE_FLOAT_EXPONENT,

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
} FSMState;

#define BUFFER_INIT_SIZE 128

// Dynamic buffer for strings
typedef struct buffer {
    char *data;
    size_t length;
    size_t capacity;
} Buffer;

// Initialize buffer
int buffer_init(Buffer *buffer) {
    buffer->data = malloc(BUFFER_INIT_SIZE);
    if (buffer->data == NULL) {
        return 0;
    }
    buffer->length = 0;
    buffer->capacity = BUFFER_INIT_SIZE;
    return 1;
}

// Update buffer with new character
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


// Return specific keyword token or identifier token
TokenType keyword_id(const char *str) {
    char *keywords[] = {"class", "if", "else", "is", "null", "return", "var", "while", "Ifj", "static", "import", "for", "Num", "String", "Null"};
    TokenType types[] = {TOK_CLASS, TOK_IF, TOK_ELSE, TOK_IS, TOK_NULL, TOK_RETURN, TOK_VAR, TOK_WHILE, TOK_IFJ, TOK_STATIC, TOK_IMPORT, TOK_FOR, TOK_TYPE_NUM, TOK_TYPE_STRING, TOK_TYPE_NULL};

    for (int i = 0; i < 14; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return types[i];
        }
    }
    return TOK_ID;
}

// Return specific built-in function token
TokenType builtin_func(const char *str) {
    char *functions[] = {"read_str", "read_num", "write", "floor", "str", "length", "substring", "strcmp", "ord", "chr"};
    TokenType types[] = {TOK_FUNC_READ_STR, TOK_FUNC_READ_NUM, TOK_FUNC_WRITE, TOK_FUNC_FLOOR, TOK_FUNC_STR, TOK_FUNC_LENGTH, TOK_FUNC_SUBSTRING, TOK_FUNC_STRCMP, TOK_FUNC_ORD, TOK_FUNC_CHR};
    for (int i = 0; i < 10; i++) {
        if (strcmp(str, functions[i]) == 0) {
            return types[i];
        }
    }
    return TOK_IFJ;
}

/*
    Get next token from input
    return 0 on success, 1 on lexical error, 99 on internal error
    free token.data.str_value if token type is TOK_CONST_STR, TOK_CONST_ML_STR, TOK_ID or TOK_GLOBAL_ID
*/
int get_next_token(Token *token) {
    int ml_string_end = 0;
    int block_comment_level = 0;
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
                
                // Numbers starting with 0 - int, hex int or float starting with 0.
                if (chr == '0') {
                    chr = getchar();
                    if (chr == 'x') {
                        chr = getchar();
                        if (isxdigit(chr)) {
                            if (!buffer_update(&buffer, chr)) {
                                return 99;
                            }
                            state = STATE_HEX_INT;
                            break;
                        }
                        ungetc(chr, stdin);
                        goto cleanup_return_1;
                    }
                    if (chr == '.') {
                        chr = getchar();
                        if (isdigit(chr)) {
                            if (!buffer_update(&buffer, '0')) {
                                return 99;
                            }
                            if (!buffer_update(&buffer, '.')) {
                                return 99;
                            }
                            if (!buffer_update(&buffer, chr)) {
                                return 99;
                            }
                            state = STATE_FLOAT_DECIMAL;
                            break;
                        }
                        ungetc(chr, stdin);
                        goto cleanup_return_1;
                    }
                    ungetc(chr, stdin);
                    if (!buffer_update(&buffer, '0')) {
                        return 99;
                    }
                    state = STATE_DEC_INT;
                    break;
                }
                // Numbers
                if (isdigit(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    state = STATE_NUM;
                    break;
                }
                
                switch (chr) {
                    // Newline and EOF
                    case '\n':
                        token->type = TOK_EOL;
                        goto cleanup_return_0;
                    case EOF:
                        token->type = TOK_EOF;
                        goto cleanup_return_0;

                    // <, <=, >, >=, =, ==, !=
                    case '<':
                        chr = getchar();
                        if (chr == '=') {
                            token->type = TOK_LTE;
                        } else {
                            ungetc(chr, stdin);
                            token->type = TOK_LT;
                        }
                        goto cleanup_return_0;
                    case '>':
                        chr = getchar();
                        if (chr == '=') {
                            token->type = TOK_GTE;
                        } else {
                            ungetc(chr, stdin);
                            token->type = TOK_GT;
                        }
                        goto cleanup_return_0;
                    case '=':
                        chr = getchar();
                        if (chr == '=') {
                            token->type = TOK_EQ;
                        } else {
                            ungetc(chr, stdin);
                            token->type = TOK_ASSIGN;
                        }
                        goto cleanup_return_0;
                    case '!':
                        chr = getchar();
                        if (chr == '=') {
                            token->type = TOK_NEQ;
                            goto cleanup_return_0;
                        } else {
                            ungetc(chr, stdin);
                            goto cleanup_return_1;
                        }

                    // Start of global id
                    case '_':
                        if (!buffer_update(&buffer, chr)) {
                            return 99;
                        }
                        chr = getchar();

                        if (chr == '_') {
                            if (!buffer_update(&buffer, chr)) {
                                return 99;
                            }
                            chr = getchar();
                            if (isalnum(chr) || chr == '_') {
                                if (!buffer_update(&buffer, chr)) {
                                    return 99;
                                }
                                state = STATE_GLOBAL_ID;

                            } else {
                                ungetc(chr, stdin);
                                goto cleanup_return_1;
                            }
                        }
                        else {
                            ungetc(chr, stdin);
                            goto cleanup_return_1;
                        }
                        break;

                    // Start of comment or division
                    case '/':
                        state = STATE_SLASH;
                        break;

                    // Start of string
                    case '\"':
                        state = STATE_QUOTE1;
                        break;

                    // Single character tokens
                    case '+':
                        token->type = TOK_PLUS;
                        goto cleanup_return_0;
                    case '-':
                        token->type = TOK_MINUS;
                        goto cleanup_return_0;
                    case '*':
                        token->type = TOK_ASTERISK;
                        goto cleanup_return_0;
                    case '{':
                        token->type = TOK_LBRACE;
                        goto cleanup_return_0;
                    case '}':
                        token->type = TOK_RBRACE;
                        goto cleanup_return_0;
                    case '(':
                        token->type = TOK_LPAR;
                        goto cleanup_return_0;
                    case ')':
                        token->type = TOK_RPAR;
                        goto cleanup_return_0;
                    case ',':
                        token->type = TOK_COMMA;
                        goto cleanup_return_0;
                    default:
                        goto cleanup_return_1;
                }
                break;

            // Identifier or keyword                
            case STATE_ID_KEYWORD:
                if (isalnum(chr) || chr == '_') {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    break;
                }
                ungetc(chr, stdin);
                if (!buffer_update(&buffer, '\0')) {
                    return 99;
                }
                token->type = keyword_id(buffer.data);
                if (token->type == TOK_IMPORT) {
                    state = STATE_PROLOG_QUOTES;
                    break;
                }
                if (token->type == TOK_IFJ) {
                    state = STATE_BUILTIN_NAMESPACE;
                    break;
                }
                if (token->type != TOK_ID) {
                    goto cleanup_return_0;
                }
                token->data.str_value = malloc(buffer.length);
                if (token->data.str_value == NULL) {
                    return 99;
                }
                strcpy(token->data.str_value, buffer.data);
                goto cleanup_return_0;
            
            // Read 'import' - ignore whitespace and newline, check for '"ifj25"'
            case STATE_PROLOG_QUOTES:
                if (isblank(chr) || chr == '\n') {
                    break;
                }
                if (chr != '\"') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                chr = getchar();
                if (chr != 'i') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                chr = getchar();
                if (chr != 'f') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                chr = getchar();
                if (chr != 'j') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                chr = getchar();
                if (chr != '2') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                chr = getchar();
                if (chr != '5') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                chr = getchar();
                if (chr != '\"') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                state = STATE_PROLOG_FOR;
                break;
            
            // Read 'import "ifj25"' - ignore whitespace, check for 'for'
            case STATE_PROLOG_FOR:
                if (isblank(chr)) {
                    break;
                }
                if (chr != 'f') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                chr = getchar();
                if (chr != 'o') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                chr = getchar();
                if (chr != 'r') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                state = STATE_PROLOG_IFJ;
                break;

            case STATE_PROLOG_IFJ:
                if (isblank(chr) || chr == '\n') {
                    break;
                }
                if (chr != 'I') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                chr = getchar();
                if (chr != 'f') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                chr = getchar();
                if (chr != 'j') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                token->type = TOK_PROLOG;
                goto cleanup_return_0;


            // Read "Ifj" - ignore whitespace, check for '.'
            case STATE_BUILTIN_NAMESPACE:
                if (chr == '.') {
                    state = STATE_BUILTIN_DOT;
                    break;
                }
                if (isblank(chr) || chr == '\n') {
                    break;
                }
                ungetc(chr, stdin);
                goto cleanup_return_1;

            // Read "Ifj." - ignore whitespace, check for function name
            case STATE_BUILTIN_DOT:
                if (isalpha(chr)) {
                    buffer.length = 0;
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    state = STATE_BUILTIN_FUNC;;
                    break;
                }
                if (isblank(chr) || chr == '\n') {
                    break;
                }
                ungetc(chr, stdin);
                goto cleanup_return_1;

            // Builtin function name after "Ifj."
            case STATE_BUILTIN_FUNC:
                if (isalnum(chr) || chr == '_') {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    break;
                }
                ungetc(chr, stdin);
                if (!buffer_update(&buffer, '\0')) {
                    return 99;
                }
                token->type = builtin_func(buffer.data);
                if (token->type == TOK_IFJ) {
                    goto cleanup_return_1;
                }
                goto cleanup_return_0;

            // Global identifier
            case STATE_GLOBAL_ID:
                if (isalnum(chr) || chr == '_') {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    break;
                }
                ungetc(chr, stdin);
                if (!buffer_update(&buffer, '\0')) {
                    return 99;
                }
                token->type = TOK_GLOBAL_ID;
                token->data.str_value = malloc(buffer.length);
                if (token->data.str_value == NULL) {
                    return 99;
                }
                strcpy(token->data.str_value, buffer.data);
                goto cleanup_return_0;

            // One quote read
            case STATE_QUOTE1:
                switch (chr) {
                    case '\n':
                    case EOF:
                        ungetc(chr, stdin);
                        goto cleanup_return_1;
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

            // String literal
            case STATE_STRING:
                switch (chr) {
                    case '\n':
                    case EOF:
                        ungetc(chr, stdin);
                        goto cleanup_return_1;
                    case '\\':
                        state = STATE_ESCAPE;
                        break;
                    case '\"':
                        if (!buffer_update(&buffer, '\0')) {
                            return 99;
                        }
                        token->type = TOK_CONST_STR;
                        token->data.str_value = malloc(buffer.length);
                        if (token->data.str_value == NULL) {
                            return 99;
                        }
                        strcpy(token->data.str_value, buffer.data);
                        goto cleanup_return_0;
                    default:
                        if (!buffer_update(&buffer, chr)) {
                            return 99;
                        }
                        break;
                }
                break;

            // Escape sequence in string
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
                            goto cleanup_return_1;
                        }
                        int digit1 = getchar();
                        if (!isxdigit(digit1)) {
                            ungetc(digit1, stdin);
                            ungetc(digit16, stdin);
                            goto cleanup_return_1;
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
                        goto cleanup_return_1;
                }
                break;

            // Two quotes read - empty string or start of multi-line string
            case STATE_QUOTE2:
                if (chr == '\"') {
                    state = STATE_ML_START_WS;
                    break;
                }
                ungetc(chr, stdin);
                token->type = TOK_CONST_STR;
                token->data.str_value = malloc(1);
                if (token->data.str_value == NULL) {
                    return 99;
                }
                token->data.str_value[0] = '\0';
                goto cleanup_return_0;
                break;

            // Check for whitespace after starting multi-line string
            case STATE_ML_START_WS:
                if (chr == EOF) {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
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

            // Multi-line string literal
            case STATE_ML_STRING:
                if (chr == EOF) {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
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

            // Check for whitespace after newline in multi-line string
            case STATE_ML_END_WS:
                if (chr == EOF) {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
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
                            token->type = TOK_CONST_ML_STR;
                            token->data.str_value = malloc(buffer.length);
                            if (token->data.str_value == NULL) {
                                return 99;
                            }
                            strcpy(token->data.str_value, buffer.data);
                            goto cleanup_return_0;
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

            // One end quote read in multi-line string
            case STATE_ENDQUOTE1:
                if (chr == EOF) {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
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

            // Two end quotes read in multi-line string
            case STATE_ENDQUOTE2:
                if (chr == EOF) {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                if (chr == '\"') {
                    if (!buffer_update(&buffer, '\0')) {
                        return 99;
                    }
                    token->type = TOK_CONST_ML_STR;
                    token->data.str_value = malloc(buffer.length);
                    if (token->data.str_value == NULL) {
                        return 99;
                    }
                    strcpy(token->data.str_value, buffer.data);
                    goto cleanup_return_0;
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

            // Slash read - comment or division
            case STATE_SLASH:
                if (chr == '/') {
                    state = STATE_LINE_COMMENT;
                    break;
                }
                if (chr == '*') {
                    block_comment_level = 1;
                    state = STATE_BLOCK_COMMENT;
                    break;
                }
                ungetc(chr, stdin);
                token->type = TOK_DIV;
                goto cleanup_return_0;

            // Line comment
            case STATE_LINE_COMMENT:
                if (chr == '\n' || chr == EOF) {
                    ungetc(chr, stdin);
                    state = STATE_START;
                }
                break;
            
            // Block comment
            case STATE_BLOCK_COMMENT:
                if (chr == EOF) {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                if (chr == '/') {
                    chr = getchar();
                    if (chr == '*') {
                        block_comment_level++;
                    }
                    break;
                }
                if (chr == '*') {
                    chr =getchar();
                    if (chr == '/') {
                        block_comment_level--;
                        if (block_comment_level == 0) {
                            state = STATE_START;
                        }
                        break;
                    }
                }
                break;

            // Decimal integer starting with 0
            case STATE_DEC_INT:
                if (isdigit(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    break;
                }
                if (chr == '.') {
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                ungetc(chr, stdin);
                if (!buffer_update(&buffer, '\0')) {
                    return 99;
                }
                token->type = TOK_CONST_INT;
                token->data.num_int_value = strtoll(buffer.data, NULL, 10);
                goto cleanup_return_0;

            // Hexadecimal integer
            case STATE_HEX_INT:
                if (isxdigit(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    break;
                }
                ungetc(chr, stdin);
                if (!buffer_update(&buffer, '\0')) {
                    return 99;
                }
                token->type = TOK_CONST_INT;
                token->data.num_int_value = strtoll(buffer.data, NULL, 16);
                goto cleanup_return_0;

            // Int or whole part of float number
            case STATE_NUM:
                if (isdigit(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    break;
                }
                if (chr == '.') {
                    chr = getchar();
                    if (isdigit(chr)) {
                        if (!buffer_update(&buffer, '.')) {
                            return 99;
                        }
                        if (!buffer_update(&buffer, chr)) {
                            return 99;
                        }
                        state = STATE_FLOAT_DECIMAL;
                        break;
                    }
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                if (chr == 'e' || chr == 'E') {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    state = STATE_FLOAT_EXP_SIGN;
                    break;
                }
                ungetc(chr, stdin);
                if (!buffer_update(&buffer, '\0')) {
                    return 99;
                }
                token->type = TOK_CONST_INT;
                token->data.num_int_value = strtoll(buffer.data, NULL, 10);
                goto cleanup_return_0;

            // Decimal part of float number
            case STATE_FLOAT_DECIMAL:
                if (isdigit(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    break;
                }
                if (chr == 'e' || chr == 'E') {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    state = STATE_FLOAT_EXP_SIGN;
                    break;
                }
                ungetc(chr, stdin);
                if (!buffer_update(&buffer, '\0')) {
                    return 99;
                }
                token->type = TOK_CONST_FLOAT;
                token->data.num_float_value = strtod(buffer.data, NULL);
                goto cleanup_return_0;

            // Exponent sign of float number
            case STATE_FLOAT_EXP_SIGN:
                if (chr == '+' || chr == '-') {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    chr = getchar();
                    if (isdigit(chr)) {
                        if (!buffer_update(&buffer, chr)) {
                            return 99;
                        }
                        state = STATE_FLOAT_EXPONENT;
                        break;
                    }
                    ungetc(chr, stdin);
                    goto cleanup_return_1;
                }
                if (isdigit(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    state = STATE_FLOAT_EXPONENT;
                    break;
                }
                ungetc(chr, stdin);
                goto cleanup_return_1;

            // Exponent part of float number
            case STATE_FLOAT_EXPONENT:
                if (isdigit(chr)) {
                    if (!buffer_update(&buffer, chr)) {
                        return 99;
                    }
                    break;
                }
                ungetc(chr, stdin);
                if (!buffer_update(&buffer, '\0')) {
                    return 99;
                }
                token->type = TOK_CONST_FLOAT;
                token->data.num_float_value = strtod(buffer.data, NULL);;
                goto cleanup_return_0;
        }
    }
    cleanup_return_0:
    free(buffer.data);
    return 0;

    cleanup_return_1:
    free(buffer.data);
    return 1;
}