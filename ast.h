#ifndef IFJ_COMPILER_AST_H
#define IFJ_COMPILER_AST_H
/*
    Implementace překladače imperativního jazyka IFJ25

    Marek Drábek:   xdrabem00
*/

#include "scanner.h"

// AST uzel pro výrazy
typedef struct Ast_t {
    Token token;          // Token uložený přímo ve struktuře
    struct Ast_t *left;
    struct Ast_t *right;
} ast_t;

// Vytvoří uzel – token se zkopíruje (po hodnotě) do node->token
ast_t *ast_create_node(const Token *token, ast_t *left, ast_t *right);

// Rekurzivně uvolní strom včetně případných stringů v tokenech
void ast_free(ast_t *root);

#endif //IFJ_COMPILER_AST_H
