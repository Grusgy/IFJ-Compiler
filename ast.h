#ifndef IFJ_COMPILER_AST_H
#define IFJ_COMPILER_AST_H

#include "scanner.h"

// Jednoduchý binární AST uzel: token + levý/pravý potomek
// Pro výrazy: vnitřní uzly = operátory, listy = ID / konstanty
typedef struct Ast_t {
    Token *token;
    struct Ast_t *left;
    struct Ast_t *right;
} ast_t;

// Vytvoření uzlu (zapouzdření malloc + přiřazení)
ast_t *ast_create_node(Token *token, ast_t *left, ast_t *right);

// Rekurzivní dealokace celého stromu
void ast_free(ast_t *root);

#endif //IFJ_COMPILER_AST_H

