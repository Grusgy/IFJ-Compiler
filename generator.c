//
// Created by Pavel Bobek on 01.12.2025.
//

#include "generator.h"

#include <stdio.h>
#include <stdlib.h>

int if_counter = 0;
int while_counter = 0;
int tempVar_counter = 0;

//Hlavní funkce pro generaci kódu
void codeGenerator(Stmt* stmt) {
    Stmt* currentStmt = stmt;

    while (currentStmt != NULL) {
        getCode(stmt);
        currentStmt = currentStmt->next;
    }
}

//Tato funkce tak vyhodnotí jeden strom
void getCode(const Stmt* stmt) {

    if (stmt == NULL) {

    }
    else if (stmt->type == STMT_BLOCK) {
        codeGenerator(stmt->as.block.first);
    }
    else if (stmt->type == STMT_IF) {
        char* ifName;
        codegen_getName(NAME_IF, "", &ifName);
        evaluate(stmt->as.if_stmt.cond);
        printJumpComparison(stmt->as.if_stmt.cond, ifName);
        codeGenerator(stmt->as.if_stmt.then_branch);
        printf("LABEL %s\n", ifName);
        codeGenerator(stmt->as.if_stmt.else_branch);
        printf("LABEL %s_\n", ifName);
    }
    else if (stmt->type == STMT_WHILE) {
        char* whileName;
        codegen_getName(NAME_WHILE, "", &whileName);
        printf("LABEL %s\n", whileName);
        evaluate(stmt->as.while_loop.cond);
        printJumpComparison(stmt->as.while_loop.cond, whileName);
        codeGenerator(stmt->as.while_loop.block);
        printf("JUMP %s\n", whileName);
        printf("LABEL %s_\n", whileName);
    }
    else if (stmt->type == STMT_VAR_DECL) {
        char* varName;
        codegen_getName(NAME_VAR, stmt->as.var_decl.var_name, &varName);
        printf("DEFVAR %s\n", varName);
    }
    else if (stmt->type == STMT_ASSIGN) {
        evaluate(stmt->as.assign.expr);
        char* varName;
        codegen_getName(NAME_VAR, stmt->as.assign.var_name, &varName);
        printf("POPS %s\n", varName);
    }
    else if (stmt->type == STMT_FUN_DECL) {
        char* funName;
        codegen_getName(NAME_FUN, stmt->as.fun_dec.fun_name, &funName);
        printf("LABEL %s\n", funName);
        printf("CREATEFRAME\n");
        printf("PUSHFRAME\n");
        printPopParams(stmt->as.fun_dec.parameters);
        codeGenerator(stmt->as.fun_dec.fun_block);
        printf("POPFRAME\n");
        printf("RETURN\n");
    }
    else if (stmt->type == STMT_FUN_CALL) {
        printPushParams(stmt->as.fun_call.parameters);
        char* funName;
        codegen_getName(NAME_FUN, stmt->as.fun_call.fun_name, &funName);
        printf("CALL %s\n", funName);
    }
    else if (stmt->type == STMT_RETURN) {
        char* varName;
        codegen_getName(NAME_VAR, stmt->as.return_.var_name, &varName);
        printf("PUSHS %s\n", varName);
    }
}

//Tato funkce má vyhodnotit nějakej příklad např když máme y=3*x+10 tak tahle funkce dostane 3*x+10 a vyhodnotí tuto pravou část rovnice
void evaluate(const ast_t* ast) {

    if (ast->token->type == TOK_VAR ||
       (ast->token->type >= TOK_CONST_INT && ast->token->type <= TOK_CONST_ML_STR) ||
       (ast->token->type >= TOK_PLUS && ast->token->type <= TOK_NEQ)) {

        if (ast->left == NULL && ast->right == NULL) {
            if (ast->token->type == TOK_VAR) {
                char* varName;
                codegen_getName(NAME_VAR, ast->token->data.str_value, &varName);
                printf("PUSHS %s\n", varName);
            }
            else {

            }
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
                    operation = "LTES";
                    break;
                case TOK_GTE:
                    operation = "GTES";
                    break;
                case TOK_EQ:
                    operation = "EQS";
                    break;
                case TOK_NEQ:
                    operation = "NEQS";
                    break;
                default: operation = "";
            }

            printf("%s\n", operation);
        }
    }
    else {
        char* funName;
        codegen_getName(NAME_FUN, ast->token->data.str_value, &funName);
        printf("CALL %s\n", funName);
    }
}

//Pomocná funkce, která na záčátku deklarace funkce vytiskne deklaraci a přiřazení parametrů
void printPopParams(const ast_t* params) {
    const long long int count = params->token->data.num_int_value;
    ast_t* currentPar = params->right;

    char* paramsName[MAX_PARAMS];

    for (int i = 0; i < count; i++) {
        codegen_getName(NAME_VAR, currentPar->token->data.str_value, &paramsName[i]);
        printf("DEFVAR %s\n", paramsName[i]);
        currentPar = currentPar->right;
    }

    for (long long int i = count-1; i >= 0; i++) {
        printf("POPS %s\n", paramsName[i]);
    }
}

//Před zavoláním funkce pushne parametry do stacku
void printPushParams(const ast_t* params) {
    const long long int count = params->token->data.num_int_value;
    ast_t* currentPar = params->right;

    for (long long int i = 0; i < count; i++) {
        char* varName;
        codegen_getName(NAME_VAR, currentPar->token->data.str_value, &varName);
        printf("PUSHS %s\n", varName);
        currentPar = currentPar->right;
    }
}

void printJumpComparison(const ast_t* ast, char* name) {
    if (ast->token->type == TOK_EQ)
        printf("JUMPIFNEQS %s_\n", name);
    else
        printf("JUMPIFEQS %s_\n", name);
}

void codegen_getName(const nameType name_type, char* currentName, char** resultName) {

    switch (name_type) {
        case NAME_IF:
            snprintf(*resultName, 5+if_counter/10, "IF$%i", if_counter);
            if_counter++;
            break;
        case NAME_WHILE:
            snprintf(*resultName, 5+while_counter/10, "IF$%i", while_counter);
            while_counter++;
            break;
        case NAME_VAR:
            snprintf(*resultName, strlen(currentName)+2, "%s$var", currentName);
            break;
        case NAME_FUN:
            snprintf(*resultName, strlen(currentName)+2, "%s$fun", currentName);
            break;
        case NAME_TEMP:
            snprintf(*resultName, 5+tempVar_counter/10, "T$%i", tempVar_counter);
            break;
        default:
            *resultName = currentName;
    }
}