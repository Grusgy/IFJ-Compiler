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
            default:
                break;
        }
        free(stmt);
        stmt = next;
    }
}
