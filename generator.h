//
// Created by Pavel Bobek on 01.12.2025.
//

#ifndef IFJ_COMPILER_GENERATOR_H
#define IFJ_COMPILER_GENERATOR_H

#include "scanner.h"


#define MAX_STACK 100
#define MAX_PARAMS 10

//Uzel stromu který bude obsahovat jeden token a ukazatel na jeho pravého a levého syna - Pavel Bobek
typedef struct Ast_t ast_t;
typedef struct symTable_t SymTable;

typedef enum {
    AST_ASSIGN,
    AST_FUN,
    AST_FUN_CALL,
    AST_GETTER
} ast_type;

void getCode(const ast_t* ast, SymTable** symTable);
ast_t* getLastNode(ast_t* ast);
void ast_delete(ast_t* ast, ast_t* astDelete);
char* getVarName(const Token* token);
void evaluate(const ast_t* ast, SymTable** symTable);
char* getNewName();
void printJumpComparison(const ast_t* ast, char* name);
void printPopParams(const ast_t* params, SymTable** sym_table);
void printPushParams(const ast_t* params, SymTable** sym_table);


#endif //IFJ_COMPILER_GENERATOR_H