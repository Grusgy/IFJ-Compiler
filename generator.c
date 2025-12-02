//
// Created by Pavel Bobek on 01.12.2025.
//

#include "generator.h"

#include <stdio.h>
#include <string.h>

//Hlavní funkce pro generaci kódu
int codeGenerator(Stmt* stmt)
{
    Stmt* currentStmt = stmt;

    while (currentStmt != NULL) {
        getCode(stmt);
        currentStmt = currentStmt->next;
    }

    return 0;
}

//Tato funkce tak vyhodnotí jeden strom
void getCode(Stmt* stmt) {

    if (stmt == NULL) {

    }
    else if (stmt->type == STMT_BLOCK) {
        codeGenerator(stmt->as.block.first);
    }
    else if (stmt->type == STMT_IF) {
        char* ifName = getNewName();
        evaluate(stmt->as.if_stmt.cond);
        printJumpComparison(stmt->as.if_stmt.cond, ifName);
        codeGenerator(stmt->as.if_stmt.then_branch);
        printf("LABEL %s\n", ifName);
        codeGenerator(stmt->as.if_stmt.else_branch);
        printf("LABEL %s_\n", ifName);
    }
    else if (stmt->type == STMT_WHILE) {
        char* whileName = getNewName();
        printf("LABEL %s\n", whileName);
        evaluate(stmt->as.while_loop.cond);
        printJumpComparison(stmt->as.while_loop.cond, whileName);
        codeGenerator(stmt->as.while_loop.block);
        printf("JUMP %s\n", whileName);
        printf("LABEL %s_\n", whileName);
    }
    else if (stmt->type == STMT_VAR_DECL) {
        printf("DEFVAR %s\n", getNewName());
    }
    else if (stmt->type == STMT_ASSIGN) {
        evaluate(stmt->as.assign.expr);
        printf("POPS %s\n", getNewName());
    }
    else if (stmt->type == STMT_FUN_DECL) {
        printf("LABEL %s\n", getNewName());
        printf("CREATEFRAME\n");
        printf("PUSHFRAME\n");
        //printPopParams(ast->right, symTable);     dodělat parametry
        codeGenerator(stmt->as.fun_dec.fun_block);
        printf("POPFRAME\n");
        printf("RETURN\n");
    }
    else if (stmt->type == STMT_FUN_CALL) {
        //printPushParams(ast->right);              dodělat parametry
        printf("CALL %s\n", getVarName());
    }
    else if (stmt->type == STMT_RETURN) {
        printf("PUSHS %s\n", getVarName());
    }
}

//Tato funkce má vyhodnotit nějakej příklad např když máme y=3*x+10 tak tahle funkce dostane 3*x+10 a vyhodnotí tuto pravou část rovnice
void evaluate(ast_t* ast) {

    if (ast->token->type == TOK_FUN) { //Zkontrolovat jestli je token id funkce nebo (proměná nebo konstatna)
        printf("CALL %s\n", getVarName(ast->token));
    }
    else {
        if (ast->left == NULL && ast->right == NULL) {
            printf("PUSHS %s\n", getVarName());
        }
        else {
            evaluate(ast->left);
            evaluate(ast->right);

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
char* getVarName() {

    return "";
}

//Taky čekám na symtable
char* getNewName() {
    return "";
}