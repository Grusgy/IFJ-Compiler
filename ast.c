#include <stdlib.h>
#include "ast.h"

ast_t *ast_create_node(Token *token, ast_t *left, ast_t *right) {
    ast_t *node = malloc(sizeof(ast_t));
    if (!node) return NULL;
    node->token = token;
    node->left = left;
    node->right = right;
    return node;
}

void ast_free(ast_t *root) {
    if (!root) return;
    ast_free(root->left);
    ast_free(root->right);
    free(root->token);   // POZOR: počítáme s tím, že token je mallocnutý
    free(root);
}
