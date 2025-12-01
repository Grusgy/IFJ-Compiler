//
// Created by Pavel Bobek on 01.12.2025.
//

#include "generator.h"
#include "scanner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Ast_t {

    Token* token;
    struct Ast_t* right;
    struct Ast_t* left;

} ast_t;

//Hlavní funkce pro generaci kódu - Pavel Bobek
int codeGenerator(ast_t** ast, SymTable** symTable, int count)
{
    Stack* stack = malloc(sizeof(Stack));
    if (stack == NULL)
        return 99;
    Stack_Init(stack);

    int returnCode = createStack(ast, count, stack);
    if (returnCode != 0)
        return returnCode;

    while (!Stack_IsEmpty(stack)) {
        instruction_t* instruction = NULL;
        Stack_Top(stack, &instruction);
        getCode(instruction, *symTable);
        Stack_Pop(stack);
    }

    return 0;
}

//Projde všechny stromy, rozdělí je na jednotlivé instrukce a přidá je do zásobníku - Pavel Bobek
int createStack(ast_t** ast, const int count, Stack* stack) {
    for (int i = 0; i < count; i++) {
        for (int j =0; ast[i]->right != NULL; j++) {
            ast_t* lastNode = getLastNode(ast[i]);

            instruction_t* instruction = malloc(sizeof(instruction_t));
            if (instruction == NULL)
                return 99;

            instruction->operatorToken = lastNode->token;
            instruction->varToken1 = lastNode->right->token;
            instruction->varToken2 = lastNode->left->token;

            if (ast[i] == lastNode)
                instruction->resultToken = ast[i]->left->token;
            else {
                lastNode->token->type = lastNode->right->token->type;
                lastNode->token->data = lastNode->right->token->data;
                lastNode->token->data.FrameType = FR_TF;
                snprintf(lastNode->token->data.str_value, 5, "T%d", j);
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




}

//Funkce vrátí ukazatel na přeposlední pravej uzel - Pavel Bobek
ast_t* getLastNode(ast_t* ast) {
    if (ast->right->right != NULL)
        return getLastNode(ast->right);
    return ast;
}

//Funkce převede token na string pro vytisknutí instrukce - Pavel Bobek
char* getVarCode(const Token* token) {
    switch (token->data.FrameType) {
        case FR_GF:
            return strcat("GF@", token->data.str_value);
        case FR_LF:
            return strcat("LF@", token->data.str_value);
        case FR_TF:
            return strcat("TF@", token->data.str_value);
    }
    return NULL;
}