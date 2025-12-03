//
// Created by Pavel Bobek on 01.12.2025.
//

#ifndef IFJ_COMPILER_GENERATOR_H
#define IFJ_COMPILER_GENERATOR_H

#include "stmt.h"

#define MAX_STACK 100
#define MAX_PARAMS 10

typedef enum {

    NAME_IF,
    NAME_WHILE,
    NAME_FUN,
    NAME_VAR,
    NAME_TEMP

} nameType;

void codeGenerator(Stmt* stmt);
void getCode(const Stmt* stmt);
void evaluate(const ast_t* ast);
void printJumpComparison(const ast_t* ast, char* name);
void printPopParams(const ast_t* params);
void printPushParams(const ast_t* params);
void codegen_getName(nameType name_type, char* currentName, char** resultName);


#endif //IFJ_COMPILER_GENERATOR_H