#ifndef IFJ_COMPILER_STMT_H
#define IFJ_COMPILER_STMT_H
/*
    Implementace překladače imperativního jazyka IFJ25

    Marek Drábek:   xdrabem00
*/


#include "ast.h"
#include "symtable.h"

// Typ příkazu
typedef enum {
    STMT_ASSIGN,
    STMT_VAR_DECL,
    STMT_IF,
    STMT_BLOCK,
    STMT_NONE,
    STMT_WHILE,
    STMT_FUN_DECL,
    STMT_FUN_CALL,
    STMT_RETURN
} StmtType;

// Dopředná deklarace, aby se Stmt mohl odkazovat na sám sebe
typedef struct Stmt Stmt;

struct Stmt {
    StmtType type;
    Stmt *next;  // další příkaz ve stejném bloku

    union {
        struct {                // var jmeno = expr
            char *var_name;
            ast_t *expr;        // může být NULL (bez inicializace)
        } var_decl;

        struct {                // x = expr
            char *var_name;
            ast_t *expr;
        } assign;

        struct {                // if (cond) then { ... } [else { ... }]
            ast_t *cond;        // výrazová podmínka
            Stmt *then_branch;  // první příkaz v těle ifu
            Stmt *else_branch;  // první příkaz v else bloku (nebo NULL)
        } if_stmt;

        struct {                // blok příkazů { stmt1; stmt2; ... }
            Stmt *first;        // první příkaz
        } block;

        struct {
            ast_t *cond;
            Stmt *block;
        } while_loop;

        struct {
            char* fun_name;
            Stmt* fun_block;
            ast_t* parameters;
        } fun_dec;

        struct {
            char* fun_name;
            ast_t* parameters;
        } fun_call;

        struct {
            char* var_name;   //Může být null v případě že funkce je void
        } return_;
    } as;
};

// Vytvoření nového uzlu příkazu
Stmt *stmt_create(StmtType type);

// Uvolnění celé větve příkazů
void stmt_free(Stmt *stmt);

#endif //IFJ_COMPILER_STMT_H
