#include <stdlib.h>
#include "ast.h"

static int token_has_heap_string(TokenType t) {
    switch (t) {
        case TOK_CONST_STR:
        case TOK_CONST_ML_STR:
        case TOK_ID:
        case TOK_GLOBAL_ID:
            return 1;
        default:
            return 0;
    }
}

ast_t *ast_create_node(const Token *token, ast_t *left, ast_t *right) {
    ast_t *node = malloc(sizeof(ast_t));
    if (!node) return NULL;

    node->token = *token;  // zkopírujeme celý Token (včetně ukazatele na str_value, pokud je)
    node->left = left;
    node->right = right;
    return node;
}

void ast_free(ast_t *root) {
    if (!root) return;

    ast_free(root->left);
    ast_free(root->right);

    if (token_has_heap_string(root->token.type) && root->token.data.str_value) {
        free(root->token.data.str_value);
    }

    free(root);
}
