//
// Created by Pavel Bobek on 01.12.2025.
//

#include "generator.h"

#include <stdio.h>
#include <string.h>

typedef struct Ast_t {

    Token* token;
    struct Ast_t* right;
    struct Ast_t* left;
    struct Ast_t* extra_ast; //Třetí větev například pro if statement nebo while loop tak v něm bude uložená kondice, můžeme změnit a dát třeba do levé branche kvůli paměti
    ast_type astType;

    //Tady teda chybí ještě pointer na další strom pro další instrukce (linked list) jak jsme se bavili na dc
    //struct Ast_t* next_tree;


} ast_t;

//Hlavní funkce pro generaci kódu
int codeGenerator(ast_t** ast, SymTable** symTable, const int count)
{

    for (int i = 0; i < count; i++) {
        getCode(ast[i], symTable);
    }

    return 0;
}

//Tato funkce tak vyhodnotí jeden strom
void getCode(const ast_t* ast, SymTable** symTable) {

    if (ast->token == NULL) {

    }
    else if (ast->token->type == TOK_IF) {
        char* ifName = getNewName();
        evaluate(ast->extra_ast, symTable);
        printJumpComparison(ast->extra_ast, ifName);
        getCode(ast->left, symTable);
        printf("LABEL %s\n", ifName);
        getCode(ast->right, symTable);
        printf("LABEL %s\n_", ifName);
    }
    else if (ast->token->type == TOK_WHILE) {
        char* whileName = getNewName();
        printf("LABEL %s\n", whileName);
        evaluate(ast->extra_ast, symTable);
        printJumpComparison(ast->extra_ast, whileName);
        getCode(ast->left, symTable);
        printf("JUMP %s\n", whileName);
        printf("LABEL %s_\n", whileName);
    }
    else if (ast->token->type == TOK_VAR) {
        printf("DEFVAR %s\n", getNewName());
    }
    else if (ast->token->type == TOK_ID) {
        if (ast->astType == AST_ASSIGN) {
            evaluate(ast->right, symTable);
            printf("POPS %s\n", getNewName());
        }
        else if (ast->astType == AST_FUN){
            printf("LABEL %s\n", getNewName());
            printf("CREATEFRAME\n");
            printf("PUSHFRAME\n");
            printPopParams(ast->right, symTable);
            getCode(ast->right, symTable);
            printf("POPFRAME\n");
            printf("RETURN\n");
        }
        else if (ast->astType == AST_FUN_CALL) {
            printPushParams(ast->right, symTable);
            printf("CALL %s\n", getVarName(ast->extra_ast->token));
        }
    }
    else if (ast->token->type == TOK_RETURN) {
        printf("PUSHS %s\n", getVarName(ast->right->token));
    }


}

//Tato funkce ještě není zdaleka hotová, budu ji dělat dneska
//Má vyhodnotit nějakej příklad např když máme y=3*x+10 tak tahle funkce dostane 3*x+10 a vyhodnotí tuto pravou část rovnice
void evaluate(const ast_t* ast, SymTable** symTable) {
    if (ast->astType == AST_ASSIGN) {
        if (ast->left == NULL && ast->right == NULL) {
            printf("PUSHS %s\n", getVarName(ast->token));
        }
        else {
            getCode(ast->left, symTable);
            getCode(ast->right, symTable);

            char* operation;

            switch (ast->token->type) {

                case TOK_PLUS:
                    operation = "ADDS";
                    break;
                case TOK_MINUS:
                    operation = "SUBS";
                    break;
                case TOK_ASTERISK:
                    operation = "MULS";
                    break;
                case TOK_DIV:
                    operation = "DIVS";
                    break;
                case TOK_LT:
                    operation = "LTS";
                    break;
                case TOK_GT:
                    operation = "GTS";
                    break;
                case TOK_LTE:
                    operation = "";
                    break;
                case TOK_GTE:
                    operation = "";
                    break;
                case TOK_EQ:
                    operation = "EQS";
                    break;
                case TOK_NEQ:
                    operation = "";
                    break;


                default: operation = "";
            }


            printf("%s\n", operation);
        }
    }
    else if (ast->astType == AST_FUN) {
        printf("CALL %s\n", getVarName(ast->token));
        printf("POPS %s\n", getNewName());
    }
}

//Pomocná funkce, která na záčátku deklarace funkce vytiskne deklaraci a přiřazení parametrů
void printPopParams(const ast_t* params, SymTable** sym_table) {
    const long long int count = params->token->data.num_int_value;
    ast_t* currentPar = params->left;

    char* paramsName[MAX_PARAMS];

    for (int i = 0; i < count; i++) {
        paramsName[i] = getNewName();
        printf("DEFVAR %s\n", paramsName[i]);
        currentPar = currentPar->left;
    }

    for (long long int i = count-1; i >= 0; i++) {
        printf("POPS %s\n", paramsName[i]);
    }
}

//Před zavoláním funkce pushne parametry do stacku
void printPushParams(const ast_t* params, SymTable** sym_table) {
    const long long int count = params->token->data.num_int_value;
    ast_t* currentPar = params->left;

    for (long long int i = 0; i < count; i++) {
        printf("PUSHS %s\n", getVarName(currentPar->token));
    }
}

//
void printJumpComparison(const ast_t* ast, char* name) {
    if (ast->token->type == TOK_EQ)
        printf("JUMPIFNEQS %s_\n", name);
    else
        printf("JUMPIFEQS %s_\n", name);
}

//Tuto funkci budu dělat taky dneska ee čekám na symtable
char* getVarName(const Token* token) {
    //switch (token->data.FrameType) {
    //    case FR_GF:
    //        return strcat("GF@", token->data.str_value);
    //    case FR_LF:
    //        return strcat("LF@", token->data.str_value);
    //    case FR_TF:
    //        return strcat("TF@", token->data.str_value);
    //}
    return NULL;
}

//Taky čekám na symtable
char* getNewName() {
    return "";
}