//
// Created by Pavel Bobek on 01.12.2025.
//

#ifndef IFJ_COMPILER_GENERATOR_H
#define IFJ_COMPILER_GENERATOR_H


#include <stdbool.h>

#include "scanner.h"


#define MAX_STACK 100

//Uzel stromu který bude obsahovat jeden token a ukazatel na jeho pravého a levého syna - Pavel Bobek
typedef struct Ast_t ast_t;

typedef struct symTable_t SymTable;


//Teorie je taková že syntaktický analyzátor bude volat lexikální analyzátor který vytvoří a vrátí postupně všechny tokeny v jedné instrukci (až po středník) - Pavel Bobek
//Potom z této jedné instrukce vytvoří strom (ast) a dá ho do pole stromů - Pavel Bobek
//Finální pole stromů pak pošle do generátoru kódu, který pak vytvoří vnitřní kód - Pavel Bobek


//Budu používat v generátoru kódu
typedef struct {

    Token* operatorToken;
    Token* varToken1;
    Token* varToken2;
    Token* resultToken;

} instruction_t;

//Přidal jsem ještě moji implementaci zásobníku z prvního projektu do IAL - Pavel Bobek
typedef struct {
    instruction_t **InstructionArray;
    int topIndex;
} Stack;

int Stack_Init( Stack * );
bool Stack_IsEmpty( const Stack * );
bool Stack_IsFull( const Stack * );
void Stack_Top( const Stack *, instruction_t ** );
void Stack_Pop( Stack * );
void Stack_Push( Stack *, instruction_t * );
void Stack_Dispose( Stack * );

//Zatím funkce pro generátor kódu - Pavel Bobek
void getCode(const instruction_t* instruction, SymTable* symTable);
ast_t* getLastNode(ast_t* ast);
int createStack(ast_t** ast, int count, Stack* stack);
void ast_delete(ast_t* ast, ast_t* astDelete);
char* getVarCode(const Token* token);


#endif //IFJ_COMPILER_GENERATOR_H