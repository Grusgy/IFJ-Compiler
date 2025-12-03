/*
    Implementace překladače imperativního jazyka IFJ25

    Marek Drábek:   xdrabem00
*/

#include <stdlib.h>
#include "stmt.h"
#include "ast.h"

Stmt *stmt_create(StmtType type) {
    Stmt *s = malloc(sizeof(Stmt));
    if (!s) return NULL;
    s->type = type;
    s->next = NULL;
    return s;
}

void stmt_free(Stmt *stmt) {
    while (stmt) {
        Stmt *next = stmt->next;
        switch (stmt->type) {
            case STMT_ASSIGN:
                free(stmt->as.assign.var_name);
                ast_free(stmt->as.assign.expr);
                break;

            case STMT_VAR_DECL:
                free(stmt->as.var_decl.var_name);
                ast_free(stmt->as.var_decl.expr);
                break;

            case STMT_IF:
                ast_free(stmt->as.if_stmt.cond);
                stmt_free(stmt->as.if_stmt.then_branch);
                stmt_free(stmt->as.if_stmt.else_branch);
                break;

            case STMT_BLOCK:
                stmt_free(stmt->as.block.first);
                break;

            case STMT_WHILE:
                ast_free(stmt->as.while_loop.cond);
                stmt_free(stmt->as.while_loop.block);
                break;

            case STMT_FUN_CALL:
                free(stmt->as.fun_call.fun_name);
                ast_free(stmt->as.fun_call.parameters);
                break;

            case STMT_FUN_DECL:
                free(stmt->as.fun_dec.fun_name);
                stmt_free(stmt->as.fun_dec.fun_block);
                ast_free(stmt->as.fun_dec.parameters);
                break;

            case STMT_RETURN:
                if (stmt->as.return_.var_name)
                    free(stmt->as.return_.var_name);
                break;

            case STMT_NONE:
            default:
                break;
        }
        free(stmt);
        stmt = next;
    }
}


