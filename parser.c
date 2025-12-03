#include <stdlib.h>
#include <string.h>
#include "parser.h"

// =================== Globální stav ===================

// Globální aktuální token – scanner ho naplňuje přes get_next_token
static Token cur_tok;

static int next_token(void) {
    return get_next_token(&cur_tok);
}

// jednoduchý helper na očekávání typu (zatím ho skoro nepoužíváme)
static int expect(TokenType t) {
    if (cur_tok.type != t) return 2;   // syntaktická chyba
    return 0;
}

// =============== Deklarace interních funkcí ===============

static int parse_statement(Stmt **out_stmt, SymTable *symtable);
static int parse_expression(ast_t **out_expr);
static int parse_block(Stmt **out_block_stmt, SymTable *parent_symtable);
static int parse_while(Stmt **out_stmt, SymTable *symtable);
static int parse_return_stmt(Stmt **out_stmt, SymTable *symtable);
static int parse_id_statement(Stmt **out_stmt, SymTable *symtable);

// ================== PARSOVÁNÍ VÝRAZŮ ======================

static int get_precedence(TokenType t) {
    switch (t) {
        case TOK_ASTERISK:
        case TOK_DIV:
            return 3;
        case TOK_PLUS:
        case TOK_MINUS:
            return 2;
        case TOK_LT:
        case TOK_GT:
        case TOK_LTE:
        case TOK_GTE:
        case TOK_EQ:
        case TOK_NEQ:
            return 1;
        default:
            return 0;
    }
}

// primární výraz: literál, identifikátor, (expr)
static int parse_primary(ast_t **out) {
    // závorky
    if (cur_tok.type == TOK_LPAR) {
        int rc = next_token();
        if (rc) return rc;

        rc = parse_expression(out);
        if (rc) return rc;

        if (cur_tok.type != TOK_RPAR) return 2;
        return next_token();
    }

    // ID / konstanta
    if (cur_tok.type == TOK_ID ||
        cur_tok.type == TOK_CONST_INT ||
        cur_tok.type == TOK_CONST_FLOAT ||
        cur_tok.type == TOK_CONST_STR ||
        cur_tok.type == TOK_CONST_ML_STR)
    {
        // Token je na zásobníku -> jen ho zkopírujeme do AST uzlu
        *out = ast_create_node(&cur_tok, NULL, NULL);
        if (!*out) return 99;

        // Vlastnictví případného str_value přechází na AST.
        return next_token();
    }

    return 2; // syntaktická chyba
}

static int parse_binop_rhs(int min_prec, ast_t *lhs, ast_t **out) {
    while (1) {
        int prec = get_precedence(cur_tok.type);
        if (prec < min_prec)
            break;

        // Operátor – KOPIE tokenu po hodnotě
        Token op_token = cur_tok;

        int rc = next_token();
        if (rc) return rc;

        ast_t *rhs = NULL;
        rc = parse_primary(&rhs);
        if (rc) return rc;

        int next_prec = get_precedence(cur_tok.type);
        if (prec < next_prec) {
            rc = parse_binop_rhs(prec + 1, rhs, &rhs);
            if (rc) return rc;
        }

        ast_t *new_lhs = ast_create_node(&op_token, lhs, rhs);
        if (!new_lhs) return 99;
        lhs = new_lhs;
    }

    *out = lhs;
    return 0;
}

static int parse_expression(ast_t **out_expr) {
    ast_t *lhs = NULL;
    int rc = parse_primary(&lhs);
    if (rc) return rc;

    return parse_binop_rhs(1, lhs, out_expr);
}

// ================== POMOCNÉ: ID → ASSIGN / FUN_CALL ======================
//
// id = expr       → STMT_ASSIGN
// id(expr, ...)  → STMT_FUN_CALL
//
static int parse_id_statement(Stmt **out_stmt, SymTable *symtable) {
    if (cur_tok.type != TOK_ID) return 2;

    // token scanneru obsahuje mallocnutý string jména
    char *id_name = cur_tok.data.str_value;

    int rc = next_token(); // posuneme se za ID
    if (rc) return rc;

    // varianta 1: přiřazení
    if (cur_tok.type == TOK_ASSIGN) {
        // sémantika: proměnná musí existovat (včetně rodičovských scope)
        SymbolData *found = symtable_find(symtable, id_name);
        if (!found || found->kind != SYM_VAR) {
            return 3; // použití nedefinované proměnné
        }

        Stmt *stmt = stmt_create(STMT_ASSIGN);
        if (!stmt) return 99;

        stmt->as.assign.var_name = id_name;  // stmt teď vlastní ten string
        stmt->as.assign.expr = NULL;

        rc = next_token();   // za '='
        if (rc) return rc;

        ast_t *expr = NULL;
        rc = parse_expression(&expr);
        if (rc) return rc;
        stmt->as.assign.expr = expr;

        // označíme jako inicializovanou
        found->data.var.initialized = true;

        *out_stmt = stmt;
        return 0;
    }

    // varianta 2: volání funkce id(...)
    if (cur_tok.type == TOK_LPAR) {
        Stmt *stmt = stmt_create(STMT_FUN_CALL);
        if (!stmt) return 99;

        stmt->as.fun_call.fun_name = id_name;   // stmt vlastní string
        stmt->as.fun_call.parameters = NULL;

        rc = next_token(); // za '('
        if (rc) return rc;

        // argumenty – 0 nebo více
        if (cur_tok.type != TOK_RPAR) {
            ast_t *params = NULL;

            // první argument
            rc = parse_expression(&params);
            if (rc) return rc;

            // další argumenty oddělené čárkou – řetěz jako binární strom s TOK_COMMA
            while (cur_tok.type == TOK_COMMA) {
                Token comma_tok = cur_tok;
                rc = next_token();
                if (rc) return rc;

                ast_t *next_arg = NULL;
                rc = parse_expression(&next_arg);
                if (rc) return rc;

                ast_t *list_node = ast_create_node(&comma_tok, params, next_arg);
                if (!list_node) return 99;
                params = list_node;
            }

            stmt->as.fun_call.parameters = params;
        }

        if (cur_tok.type != TOK_RPAR) return 2;
        rc = next_token(); // za ')'
        if (rc) return rc;

        *out_stmt = stmt;
        return 0;
    }

    // nic z toho – chyba
    return 2;
}

// ================== PARSOVÁNÍ PŘÍKAZŮ ======================

// var x [= expr]
static int parse_var_decl(Stmt **out_stmt, SymTable *symtable) {
    // cur_tok == TOK_VAR
    int rc = next_token();    // přeskočíme "var"
    if (rc) return rc;

    if (cur_tok.type != TOK_ID) return 2;
    char *id_name = cur_tok.data.str_value;  // ukazatel z tokenu – předáme ho dál

    // sémantika: nesmí být redefinice ve STEJNÉM scope
    if (symtable_find_here(symtable, id_name) != NULL) {
        return 3; // sémantická chyba – redeklarace ve stejném scope
    }

    // vytvoříme Stmt pro deklaraci
    Stmt *stmt = stmt_create(STMT_VAR_DECL);
    if (!stmt) return 99;

    // Jméno proměnné – Stmt převezme vlastnictví stringu z tokenu
    stmt->as.var_decl.var_name = id_name;
    stmt->as.var_decl.expr = NULL;

    // vytvoříme symbol – typ zatím TYPE_UNDEF, není inicializovaná
    SymbolData sym = symbol_make_var(TYPE_UNDEF, false);
    if (!symtable_insert_here(symtable, id_name, sym)) return 99;

    rc = next_token();  // jdeme za ID
    if (rc) return rc;

    // var x = expr
    if (cur_tok.type == TOK_ASSIGN) {
        rc = next_token();
        if (rc) return rc;

        ast_t *expr = NULL;
        rc = parse_expression(&expr);
        if (rc) return rc;
        stmt->as.var_decl.expr = expr;

        // proměnnou označíme jako inicializovanou
        SymbolData *found = symtable_find(symtable, stmt->as.var_decl.var_name);
        if (found && found->kind == SYM_VAR) {
            found->data.var.initialized = true;
        }
    }

    *out_stmt = stmt;
    return 0;
}

// while (cond) { ... }
static int parse_while(Stmt **out_stmt, SymTable *symtable) {
    // cur_tok == TOK_WHILE
    Stmt *stmt = stmt_create(STMT_WHILE);
    if (!stmt) return 99;
    stmt->as.while_loop.cond = NULL;
    stmt->as.while_loop.block = NULL;

    int rc = next_token(); // za "while"
    if (rc) return rc;

    if (cur_tok.type != TOK_LPAR) return 2;
    rc = next_token(); // za '('
    if (rc) return rc;

    rc = parse_expression(&stmt->as.while_loop.cond);
    if (rc) return rc;

    if (cur_tok.type != TOK_RPAR) return 2;
    rc = next_token(); // za ')'
    if (rc) return rc;

    // tělo cyklu – očekáváme blok { ... }
    rc = parse_block(&stmt->as.while_loop.block, symtable);
    if (rc) return rc;

    *out_stmt = stmt;
    return 0;
}

// return [id]
static int parse_return_stmt(Stmt **out_stmt, SymTable *symtable) {
    (void)symtable; // zatím nepoužité

    // cur_tok == TOK_RETURN
    Stmt *stmt = stmt_create(STMT_RETURN);
    if (!stmt) return 99;
    stmt->as.return_.var_name = NULL;

    int rc = next_token(); // za "return"
    if (rc) return rc;

    // return bez hodnoty – další token je EOL, '}', EOF...
    if (cur_tok.type == TOK_EOL ||
        cur_tok.type == TOK_RBRACE ||
        cur_tok.type == TOK_EOF)
    {
        *out_stmt = stmt;
        return 0;
    }

    // jednoduchá varianta: return <id>
    if (cur_tok.type == TOK_ID) {
        char *id_name = cur_tok.data.str_value;
        stmt->as.return_.var_name = id_name;  // stmt vlastní string
        rc = next_token();
        if (rc) return rc;
        *out_stmt = stmt;
        return 0;
    }

    // pokud bys chtěl return libovolného výrazu, musel bys změnit
    // typ v Stmt na ast_t* místo char*.

    return 2;
}

// { stmt* }
static int parse_block(Stmt **out_block_stmt, SymTable *parent_symtable) {
    if (cur_tok.type != TOK_LBRACE) return 2;

    int rc = next_token();   // za '{'
    if (rc) return rc;

    // nový lokální scope pro tento blok
    SymTable local;
    symtable_init_child(&local, parent_symtable);

    Stmt *first = NULL;
    Stmt *last = NULL;

    // blok končí na '}' nebo EOF
    while (cur_tok.type != TOK_RBRACE && cur_tok.type != TOK_EOF) {
        // přeskočíme případné EOL
        while (cur_tok.type == TOK_EOL) {
            rc = next_token();
            if (rc) {
                symtable_free(&local);
                return rc;
            }
        }
        if (cur_tok.type == TOK_RBRACE || cur_tok.type == TOK_EOF) break;

        Stmt *stmt = NULL;
        rc = parse_statement(&stmt, &local);  // používáme lokální tabulku
        if (rc) {
            symtable_free(&local);
            return rc;
        }
        if (!stmt) break; // nic se nenaparsovalo

        if (!first) first = stmt;
        else last->next = stmt;

        // posuneme se na konec spojového seznamu
        while (stmt->next) stmt = stmt->next;
        last = stmt;
    }

    if (cur_tok.type != TOK_RBRACE) {
        symtable_free(&local);
        return 2;
    }

    rc = next_token(); // za '}'
    if (rc) {
        symtable_free(&local);
        return rc;
    }

    Stmt *block_stmt = stmt_create(STMT_BLOCK);
    if (!block_stmt) {
        symtable_free(&local);
        return 99;
    }
    block_stmt->as.block.first = first;

    *out_block_stmt = block_stmt;

    // lokální symboly už nepotřebujeme – scope končí
    symtable_free(&local);

    return 0;
}

// if (cond) { ... } [else { ... }]
static int parse_if(Stmt **out_stmt, SymTable *symtable) {
    // cur_tok == TOK_IF
    Stmt *stmt = stmt_create(STMT_IF);
    if (!stmt) return 99;
    stmt->as.if_stmt.cond = NULL;
    stmt->as.if_stmt.then_branch = NULL;
    stmt->as.if_stmt.else_branch = NULL;

    int rc = next_token();  // za "if"
    if (rc) return rc;

    if (cur_tok.type != TOK_LPAR) return 2;
    rc = next_token();      // za '('
    if (rc) return rc;

    rc = parse_expression(&stmt->as.if_stmt.cond);
    if (rc) return rc;

    if (cur_tok.type != TOK_RPAR) return 2;
    rc = next_token();      // za ')'
    if (rc) return rc;

    // THEN větev – očekáváme blok
    rc = parse_block(&stmt->as.if_stmt.then_branch, symtable);
    if (rc) return rc;

    // ELSE větev (volitelná)
    if (cur_tok.type == TOK_ELSE) {
        rc = next_token();  // za "else"
        if (rc) return rc;
        rc = parse_block(&stmt->as.if_stmt.else_branch, symtable);
        if (rc) return rc;
    }

    *out_stmt = stmt;
    return 0;
}

// jeden příkaz (bez EOL na začátku)
static int parse_statement(Stmt **out_stmt, SymTable *symtable) {
    *out_stmt = NULL;

    switch (cur_tok.type) {
        case TOK_VAR:
            return parse_var_decl(out_stmt, symtable);

        case TOK_ID:
            return parse_id_statement(out_stmt, symtable);

        case TOK_IF:
            return parse_if(out_stmt, symtable);

        case TOK_WHILE:
            return parse_while(out_stmt, symtable);

        case TOK_RETURN:
            return parse_return_stmt(out_stmt, symtable);

        case TOK_LBRACE:
            return parse_block(out_stmt, symtable);

        case TOK_EOF:
            return 0; // nic k parsování

        default:
            return 2; // syntaktická chyba – nečekaný token na začátku příkazu
    }
}

// ================== HLAVNÍ FUNKCE PARSERU ==================

int parse_program(Stmt **stmts_out, SymTable *symtable) {
    *stmts_out = NULL;
    symtable_init(symtable);   // globální scope

    int rc = next_token();
    if (rc) return rc;

    Stmt *first = NULL;
    Stmt *last = NULL;

    while (cur_tok.type != TOK_EOF) {
        // přeskočíme prázdné řádky
        while (cur_tok.type == TOK_EOL) {
            rc = next_token();
            if (rc) return rc;
        }
        if (cur_tok.type == TOK_EOF) break;

        Stmt *stmt = NULL;
        rc = parse_statement(&stmt, symtable);
        if (rc) return rc;

        if (!stmt) break;

        if (!first) first = stmt;
        else last->next = stmt;

        // posun na konec seznamu (kvůli if/block, které můžou mít v sobě další next)
        while (stmt->next) stmt = stmt->next;
        last = stmt;

        // po příkazu případně přeskočíme jeden EOL
        if (cur_tok.type == TOK_EOL) {
            rc = next_token();
            if (rc) return rc;
        }
    }

    *stmts_out = first;
    return 0;
}
