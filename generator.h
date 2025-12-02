//
// Created by Pavel Bobek on 01.12.2025.
//

#ifndef IFJ_COMPILER_GENERATOR_H
#define IFJ_COMPILER_GENERATOR_H

#include "stmt.h"


#define MAX_STACK 100
#define MAX_PARAMS 10

//Uzel stromu který bude obsahovat jeden token a ukazatel na jeho pravého a levého syna - Pavel Bobek

int codeGenerator(Stmt* stmt);
void getCode(Stmt* stmt);
char* getVarName();
void evaluate(ast_t* ast);
char* getNewName();
void printJumpComparison(const ast_t* ast, char* name);
void printPopParams(const ast_t* params, SymTable** sym_table);
void printPushParams(const ast_t* params, SymTable** sym_table);


#endif //IFJ_COMPILER_GENERATOR_H