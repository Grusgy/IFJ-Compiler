/*
Implementace překladače imperativního jazyka IFJ25

    Pavel Bobek:    xbobekp00
*/

#include "generator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool firstRun = true;

//Hlavní funkce pro generaci kódu
void codeGenerator(Stmt* stmt) {
    if(firstRun){
        printf(".IFJcode25\n");
        printf("JUMP main$fun\n\n");
        firstRun = false;
    }

    Stmt* currentStmt = stmt;

    //Prochází linked list stromů z instrukcemi (každý strom = jedna instrukce)
    while (currentStmt != NULL) {
        getCode(currentStmt);
        currentStmt = currentStmt->next;
    }
}

//Tato funkce tak vyhodnotí jeden strom
void getCode(const Stmt* stmt) {

    //V případě if statementu tak strom obsahuje block then částí a block else části
    if (stmt->type == STMT_BLOCK) {
        codeGenerator(stmt->as.block.first);
    }
    //Vyhodnocení if statementu
    else if (stmt->type == STMT_IF) {
        char* ifName;
        codegen_getName(NAME_IF, "", &ifName);
        evaluate(stmt->as.if_stmt.cond);
        char* tempVal;
        codegen_getName(NAME_TEMP, "", &tempVal);
        printf("DEFVAR %s\n", tempVal);
        printf("POPS %s\n", tempVal);
        printJumpComparison(stmt->as.if_stmt.cond, ifName, tempVal, "false");
        codeGenerator(stmt->as.if_stmt.then_branch);
        printf("JUMP %s\n", ifName);
        printf("LABEL %s_\n", ifName);
        codeGenerator(stmt->as.if_stmt.else_branch);
        printf("LABEL %s\n", ifName);
        free(ifName);
    }
    //Vyhodnocení while statementu
    else if (stmt->type == STMT_WHILE) {
        char* whileName;
        codegen_getName(NAME_WHILE, "", &whileName);
        printf("LABEL %s\n", whileName);
        evaluate(stmt->as.while_loop.cond);
        char* tempVal;
        codegen_getName(NAME_TEMP, "", &tempVal);
        printf("DEFVAR %s\n", tempVal);
        printf("POPS %s\n", tempVal);
        printJumpComparison(stmt->as.while_loop.cond, whileName, tempVal, "false");
        codeGenerator(stmt->as.while_loop.block);
        printf("JUMP %s\n", whileName);
        printf("LABEL %s_\n", whileName);
        free(whileName);
    }
    //vyhodnocení deklarace proměnné a popřípadě přiřazení hodnoty
    else if (stmt->type == STMT_VAR_DECL) {
        char* varName;
        codegen_getName(NAME_VAR, stmt->as.var_decl.var_name, &varName);
        printf("DEFVAR %s\n", varName);
        if(stmt->as.var_decl.expr != NULL) {
            evaluate(stmt->as.var_decl.expr);
            printf("POPS %s\n", varName);
        }
        free(varName);
    }
    //Přiřazení hodnoty z rovnice do proměnné
    else if (stmt->type == STMT_ASSIGN) {
        evaluate(stmt->as.assign.expr);
        char* varName;
        codegen_getName(NAME_VAR, stmt->as.assign.var_name, &varName);
        printf("POPS %s\n", varName);
        free(varName);
    }
    //Udělá rám funkce a jeho blok dá do fronty stromů
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
        free(funName);
    }
    //Pushne parametry pro funkci a zavolá ji
    else if (stmt->type == STMT_FUN_CALL) {
        printPushParams(stmt->as.fun_call.parameters);
        char* funName;
        codegen_getName(NAME_FUN, stmt->as.fun_call.fun_name, &funName);
        printf("CALL %s\n", funName);
        free(funName);
    }
    //pushne návratovou hodnotu
    else if (stmt->type == STMT_RETURN) {
        char* varName;
        codegen_getName(NAME_VAR, stmt->as.return_.var_name, &varName);
        printf("PUSHS %s\n", varName);
        free(varName);
    }
}

//Tato funkce má vyhodnotit nějakej příklad např když máme y=3*x+10 tak tahle funkce dostane 3*x+10 a vyhodnotí tuto pravou část rovnice
void evaluate(const ast_t* ast) {

    if (ast == NULL)
        return;

    //Vyhodnotí jestli je token konstanta, proměnná nebo znaménko
    if ((ast->token.type == TOK_ID && ast->right == NULL) ||
       (ast->token.type >= TOK_CONST_INT && ast->token.type <= TOK_CONST_ML_STR) ||
       (ast->token.type >= TOK_PLUS && ast->token.type <= TOK_NEQ)) {

        //V případě že je token list tak je buď proměnná nebo konstanta
        if (ast->left == NULL && ast->right == NULL) {
            //Pushne proměnnou na zásobník
            if (ast->token.type == TOK_ID) {
                char* varName;
                codegen_getName(NAME_VAR, ast->token.data.str_value, &varName);
                printf("PUSHS %s\n", varName);
                free(varName);
            }
            //Pushne konstantu na zásobník
            else {
                printf("PUSHS %lld\n", ast->token.data.num_int_value);
            }
        }
        //V tommto případě je token znaménko tak jenom uděláme jeho operaci nad zásobníkem
        else {
            //Celá funkce se bude opakovat rekurzivně dokud se rovnice nevyřeší
            evaluate(ast->left);
            evaluate(ast->right);

            char* operation;

            switch (ast->token.type) {

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
    //Speciální vestavěné funkce
    else if (ast->token.type == TOK_FUNC_WRITE) {
        printf("WRITE %s\n", ast->right->right->token.data.str_value);
    }
    else if (ast->token.type == TOK_FUNC_READ_NUM) {
        char* tempVal;
        codegen_getName(NAME_TEMP, "", &tempVal);
        printf("DEFVAR %s\n", tempVal);
        printf("READ %s float\n", tempVal);
        printf("PUSHS %s\n", tempVal);
        free(tempVal);
    }
    else if (ast->token.type == TOK_FUNC_CHR) {
        char* tempVal;
        char* tempVal2;
        codegen_getName(NAME_TEMP, "", &tempVal);
        codegen_getName(NAME_TEMP, "", &tempVal2);
        printf("DEFVAR %s\n", tempVal);
        printf("DEFVAR %s\n", tempVal2);
        if (ast->right->right->token.type == TOK_CONST_INT)
            printf("MOVE %s %s\n", tempVal, ast->right->right->token.data.num_int_value);
        else
            printf("MOVE %s %s\n", tempVal, ast->right->right->token.data.str_value);
        printf("INT2STRING %s %s\n", tempVal2, tempVal);
        printf("PUSHS %s\n", tempVal2);
        free(tempVal);
        free(tempVal2);
    }
    else if (ast->token.type == TOK_FUNC_FLOOR) {
        char* tempVal;
        codegen_getName(NAME_TEMP, "", &tempVal);
        printf("DEFVAR %s\n", tempVal);
        if (ast->right->right->token.type == TOK_CONST_FLOAT)
            printf("FLOAT2INT %s %s\n",tempVal, ast->right->right->token.data.num_float_value);
        else
            printf("MOVE %s %s\n", tempVal, ast->right->right->token.data.str_value);
        printf("PUSHS %s\n", tempVal);
        free(tempVal);
    }
    else if (ast->token.type == TOK_FUNC_LENGTH) {

    }
    else if (ast->token.type == TOK_FUNC_ORD) {
        char* tempVal;
        codegen_getName(NAME_TEMP, "", &tempVal);
        printf("DEFVAR %s\n", tempVal);
        if (ast->right->right->right->token.type == TOK_CONST_INT)
            printf("STRI2INT %s %s %s\n", tempVal, ast->right->right->token.data.str_value, ast->right->right->right->token.data.num_int_value);
        else
            printf("STRI2INT %s %s %s\n", tempVal, ast->right->right->token.data.str_value, ast->right->right->right->token.data.str_value);
        printf("PUSHS %s\n", tempVal);
        free(tempVal);
    }
    else if (ast->token.type == TOK_FUNC_READ_STR) {
        char* tempVal;
        codegen_getName(NAME_TEMP, "", &tempVal);
        printf("DEFVAR %s\n", tempVal);
        printf("READ %s string\n", tempVal);
        printf("PUSHS %s\n", tempVal);
        free(tempVal);
    }
    else if (ast->token.type == TOK_FUNC_STR) {
        char* tempVal;
        codegen_getName(NAME_TEMP, "", &tempVal);
        printf("DEFVAR %s\n", tempVal);
        if (ast->right->right->token.type == TOK_CONST_FLOAT)
            printf("FLOAT2STR %s %s\n", tempVal, ast->right->right->token.data.num_float_value);
        else
            printf("FLOAT2STR %s %s\n", tempVal, ast->right->right->token.data.str_value);
        printf("WRITE %s\n", tempVal);
        free(tempVal);
    }
    else if (ast->token.type == TOK_FUNC_STRCMP) {

    }
    else if (ast->token.type == TOK_FUNC_SUBSTRING) {

    }
    //V případě že token je funkce zavoláme ji a její návratová hodnota zůstane na zásobníku
    else {
        char* funName;
        codegen_getName(NAME_FUN, ast->token.data.str_value, &funName);
        printf("CALL %s\n", funName);
        free(funName);
    }
}

//Pomocná funkce, která na záčátku deklarace funkce vytiskne deklaraci a přiřazení parametrů
void printPopParams(const ast_t* params) {
    if (params == NULL)
        return;

    const long long int count = params->token.data.num_int_value;
    ast_t* currentPar = params->right;

    char* paramsName[MAX_PARAMS];

    //Deklrace proměnných
    for (int i = 0; i < count; i++) {
        codegen_getName(NAME_VAR, currentPar->token.data.str_value, &paramsName[i]);
        printf("DEFVAR %s\n", paramsName[i]);
        currentPar = currentPar->right;
    }

    //Přiřazení proměnných
    for (long long int i = count-1; i <= 0; i++) {
        printf("POPS %s\n", paramsName[i]);
        free(paramsName[i]);
    }
}

//Před zavoláním funkce pushne parametry do stacku
void printPushParams(const ast_t* params) {
    if (params == NULL)
        return;

    const long long int count = params->token.data.num_int_value;
    ast_t* currentPar = params->right;

    for (long long int i = 0; i < count; i++) {
        char* varName;
        codegen_getName(NAME_VAR, currentPar->token.data.str_value, &varName);
        printf("PUSHS %s\n", varName);
        currentPar = currentPar->right;
        free(varName);
    }
}

//Pomocná funkce pro if a while která vytiskne porovnání dvou symbolů nebo proměnných
void printJumpComparison(const ast_t* ast, char* label, char* symb1, char* symb2) {
    if (ast->token.type == TOK_EQ)
        printf("JUMPIFNEQ %s_ %s %s\n", label, symb1, symb2);
    else
        printf("JUMPIFEQ %s_ %s %s\n", label, symb1, symb2);
}

void codegen_getName(const nameType name_type, char* currentName, char** resultName) {

    //Globální countery
    static int if_counter = 0;
    static int while_counter = 0;
    static int tempVar_counter = 0;

    //Pro if a while a dočasnou proměnnou vrátí unikátní jméno
    //Pro funkci a proměnnou vrátí jejich speciální jméno
    switch (name_type) {
        case NAME_IF:
            *resultName = malloc(5+if_counter/10);
            snprintf(*resultName, 5+if_counter/10, "IF$%i", if_counter);
            if_counter++;
            break;
        case NAME_WHILE:
            *resultName = malloc(8+while_counter/10);
            snprintf(*resultName, 8+while_counter/10, "WHILE$%i", while_counter);
            while_counter++;
            break;
        case NAME_VAR:
            *resultName = malloc(strlen(currentName)+8);
            if (currentName[0] == '_' && currentName[1] == '_')
                snprintf(*resultName, strlen(currentName)+8, "GF_%s$var", currentName);
            else
                snprintf(*resultName, strlen(currentName)+8, "LF_%s$var", currentName);
            break;
        case NAME_FUN:
            *resultName = malloc(strlen(currentName)+5);
            snprintf(*resultName, strlen(currentName)+5, "%s$fun", currentName);
            break;
        case NAME_TEMP:
            *resultName = malloc(7+tempVar_counter/10);
            snprintf(*resultName, 7+tempVar_counter/10, "LF_T$%i", tempVar_counter);
            tempVar_counter++;
            break;
        default:
            *resultName = currentName;
    }
}