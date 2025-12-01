//
// Created by Marek on 30.11.2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"










//Hlavní funkce pro generaci kódu - Pavel Bobek
void codeGenerator(ast_t** ast, SymTable** symTable, int count)
{
    Stack* stack = malloc(sizeof(Stack));
    Stack_Init(stack);
    if (createStack(ast, count, stack) != 0)
        return;

    while (!Stack_IsEmpty(stack)) {
        instruction_t* instruction = NULL;
        Stack_Top(stack, &instruction);
        getCode(instruction, *symTable);
        Stack_Pop(stack);
    }
}

//Projde všechny stromy, rozdělí je na jednotlivé instrukce a přidá je do zásobníku - Pavel Bobek
int createStack(ast_t** ast, const int count, Stack* stack) {
    for (int i = 0; i < count; i++) {
        for (int j =0; ast[i]->right != NULL; j++) {
            ast_t* lastNode = getLastNode(ast[i]);

            instruction_t* instruction = malloc(sizeof(instruction_t));
            if (instruction == NULL)
                return 1;

            instruction->operatorToken = lastNode->token;
            instruction->varToken1 = lastNode->right->token;
            instruction->varToken2 = lastNode->left->token;

            if (ast[i] == lastNode)
                instruction->resultToken = ast[i]->left->token;
            else {
                lastNode->token->tokenType = lastNode->right->token->tokenType;
                lastNode->token->variable->variableType = lastNode->right->token->variable->variableType;
                lastNode->token->variable->frameType = TF;
                snprintf(lastNode->token->variable->varName, 5, "T%d", j);
                instruction->resultToken = lastNode->token;
            }

            Stack_Push(stack, instruction);

            ast_delete(ast[i], lastNode->left);
            ast_delete(ast[i], lastNode->right);
        }
    }

    return 0;
}

//Funkce vytiskne danou instrukci - Pavel Bobek
//Funkce není dodělaná - Pavel Bobek
void getCode(const instruction_t* instruction, SymTable* symTable) {

    if (instruction->operatorToken->operatorType == ADD) {
        printf("ADD %s %s %s", getVarCode(instruction->resultToken), getVarCode(instruction->varToken1), getVarCode(instruction->varToken2));
    }


}

//Funkce vrátí ukazatel na přeposlední pravej uzel - Pavel Bobek
ast_t* getLastNode(ast_t* ast) {
    if (ast->right->right != NULL)
        return getLastNode(ast->right);
    return ast;
}

//Funkce převede token na string pro vytisknutí instrukce - Pavel Bobek
char* getVarCode(const token_t* token) {
    if (token->tokenType != VARIABLE) {
        return NULL;
    }

    switch (token->variable->frameType) {
        case GF:
            return strcat("GF@", token->variable->varName);
        case LF:
            return strcat("LF@", token->variable->varName);
        case TF:
            return strcat("TF@", token->variable->varName);
    }
    return NULL;
}