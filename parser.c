#include <stdlib.h>
#include <string.h>
#include "parser.h"

// =================== Globální stav ===================

// aktuální token
static Token *cur_tok = NULL;

// načte další token ze scanneru
static int next_token(void) {
    if (!cur_tok) {
        cur_tok = malloc(sizeof(Token));
        if (!cur_tok) return 99;
    }
    return get_next_token(&cur_tok);    // 0 OK, 1 lex error, 99 internal
}

// jednoduchý helper na očekávání typu
static int expect(TokenType t) {
    if (cur_tok->type != t) return 2;   // syntaktická chyba
    return 0;
}

// =============== Deklarace interních funkcí ===============

static int parse_statement(Stmt **out_stmt, SymTable *symtable);
static int parse_expression(ast_t **out_expr);
static int parse_block(Stmt **out_block_stmt, SymTable *symtable);

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
    if (cur_tok->type == TOK_LPAR) {
        int rc = next_token();
        if (rc) return rc;

        rc = parse_expression(out);
        if (rc) return rc;

        if (cur_tok->type != TOK_RPAR) return 2;
        return next_token();
    }

    // ID / konstanta
    if (cur_tok->type == TOK_ID ||
        cur_tok->type == TOK_CONST_INT ||
        cur_tok->type == TOK_CONST_FLOAT ||
        cur_tok->type == TOK_CONST_STR)
    {
        Token *tok_copy = malloc(sizeof(Token));
        if (!tok_copy) return 99;
        *tok_copy = *cur_tok;     // shallow copy; pro stringy by bylo fajn časem dělat strdup

        *out = ast_create_node(tok_copy, NULL, NULL);
        if (!*out) return 99;

        return next_token();
    }

    return 2; // syntaktická chyba
}

static int parse_binop_rhs(int min_prec, ast_t *lhs, ast_t **out) {
    while (1) {
        int prec = get_precedence(cur_tok->type);
        if (prec < min_prec)
            break;

        Token *op_tok = malloc(sizeof(Token));
        if (!op_tok) return 99;
        *op_tok = *cur_tok;

        int rc = next_token();
        if (rc) return rc;

        ast_t *rhs = NULL;
        rc = parse_primary(&rhs);
        if (rc) return rc;

        int next_prec = get_precedence(cur_tok->type);
        if (prec < next_prec) {
            rc = parse_binop_rhs(prec + 1, rhs, &rhs);
            if (rc) return rc;
        }

        ast_t *new_lhs = ast_create_node(op_tok, lhs, rhs);
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

// ================== PARSOVÁNÍ PŘÍKAZŮ ======================

// var x [= expr]
static int parse_var_decl(Stmt **out_stmt, SymTable *symtable) {
    // cur_tok == TOK_VAR
    int rc = next_token();    // přeskočíme "var"
    if (rc) return rc;

    if (cur_tok->type != TOK_ID) return 2;
    char *id_name = cur_tok->data.str_value;

    // sémantika: nesmí být redefinice
    if (symtable_find(symtable, id_name) != NULL) {
        return 3; // sémantická chyba – redeklarace
    }

    // vytvoříme Stmt pro deklaraci
    Stmt *stmt = stmt_create(STMT_VAR_DECL);
    if (!stmt) return 99;
    stmt->as.var_decl.var_name = strdup(id_name);
    stmt->as.var_decl.expr = NULL;

    // vytvoříme symbol – typ zatím TYPE_UNDEF, není inicializovaná
    SymbolData sym = symbol_make_var(TYPE_UNDEF, false);
    if (!symtable_insert(symtable, id_name, sym)) return 99;

    rc = next_token();  // jdeme za ID
    if (rc) return rc;

    // var x = expr
    if (cur_tok->type == TOK_ASSIGN) {
        rc = next_token();
        if (rc) return rc;

        ast_t *expr = NULL;
        rc = parse_expression(&expr);
        if (rc) return rc;
        stmt->as.var_decl.expr = expr;

        // proměnnou označíme jako inicializovanou
        SymbolData *found = symtable_find(symtable, id_name);
        if (found && found->kind == SYM_VAR) {
            found->data.var.initialized = true;
            // typ by se tu dal odvodit z literálu, pro jednoduchost necháme TYPE_UNDEF
        }
    }

    *out_stmt = stmt;
    return 0;
}

// x = expr
static int parse_assignment(Stmt **out_stmt, SymTable *symtable) {
    if (cur_tok->type != TOK_ID) return 2;
    char *id_name = cur_tok->data.str_value;

    // sémantika: proměnná musí existovat
    SymbolData *found = symtable_find(symtable, id_name);
    if (!found || found->kind != SYM_VAR) {
        return 3; // sémantická chyba – použití nedefinované proměnné
    }

    Stmt *stmt = stmt_create(STMT_ASSIGN);
    if (!stmt) return 99;
    stmt->as.assign.var_name = strdup(id_name);
    stmt->as.assign.expr = NULL;

    int rc = next_token();   // za ID
    if (rc) return rc;

    if (cur_tok->type != TOK_ASSIGN) return 2;

    rc = next_token();       // za '='
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

// { stmt* }
static int parse_block(Stmt **out_block_stmt, SymTable *symtable) {
    if (cur_tok->type != TOK_LBRACE) return 2;

    int rc = next_token();   // za '{'
    if (rc) return rc;

    Stmt *first = NULL;
    Stmt *last = NULL;

    // blok končí na '}' nebo EOF
    while (cur_tok->type != TOK_RBRACE && cur_tok->type != TOK_EOF) {
        // přeskočíme případné EOL
        while (cur_tok->type == TOK_EOL) {
            rc = next_token();
            if (rc) return rc;
        }
        if (cur_tok->type == TOK_RBRACE || cur_tok->type == TOK_EOF) break;

        Stmt *stmt = NULL;
        rc = parse_statement(&stmt, symtable);
        if (rc) return rc;
        if (!stmt) break; // nic se nenaparsovalo

        if (!first) first = stmt;
        else last->next = stmt;

        // posuneme se na konec spojového seznamu
        while (stmt->next) stmt = stmt->next;
        last = stmt;
    }

    if (cur_tok->type != TOK_RBRACE) return 2;

    rc = next_token(); // za '}'
    if (rc) return rc;

    Stmt *block_stmt = stmt_create(STMT_BLOCK);
    if (!block_stmt) return 99;
    block_stmt->as.block.first = first;

    *out_block_stmt = block_stmt;
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

    if (cur_tok->type != TOK_LPAR) return 2;
    rc = next_token();      // za '('
    if (rc) return rc;

    rc = parse_expression(&stmt->as.if_stmt.cond);
    if (rc) return rc;

    if (cur_tok->type != TOK_RPAR) return 2;
    rc = next_token();      // za ')'
    if (rc) return rc;

    // THEN větev – očekáváme blok
    rc = parse_block(&stmt->as.if_stmt.then_branch, symtable);
    if (rc) return rc;

    // ELSE větev (volitelná)
    if (cur_tok->type == TOK_ELSE) {
        rc = next_token();  // za "else"
        if (rc) return rc;
        rc = parse_block(&stmt->as.if_stmt.else_branch, symtable);
        if (rc) return rc;
    }

    *out_stmt = stmt;
    return 0;
}

// jeden příkaz (bez EOL na začátku) – var, přiřazení, if, blok
static int parse_statement(Stmt **out_stmt, SymTable *symtable) {
    *out_stmt = NULL;

    switch (cur_tok->type) {
        case TOK_VAR:
            return parse_var_decl(out_stmt, symtable);

        case TOK_ID:
            return parse_assignment(out_stmt, symtable);

        case TOK_IF:
            return parse_if(out_stmt, symtable);

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
    symtable_init(symtable);

    int rc = next_token();
    if (rc) return rc;

    Stmt *first = NULL;
    Stmt *last = NULL;

    while (cur_tok->type != TOK_EOF) {
        // přeskočíme prázdné řádky
        while (cur_tok->type == TOK_EOL) {
            rc = next_token();
            if (rc) return rc;
        }
        if (cur_tok->type == TOK_EOF) break;

        Stmt *stmt = NULL;
        rc = parse_statement(&stmt, symtable);
        if (rc) return rc;

        if (!stmt) break;

        if (!first) first = stmt;
        else last->next = stmt;

        // posun na konec seznamu (kvůli if/block, které můžou mít v sobě další next)
        while (stmt->next) stmt = stmt->next;
        last = stmt;

        // po příkazu případně přeskočíme jeden EOL (řádkový jazyk)
        if (cur_tok->type == TOK_EOL) {
            rc = next_token();
            if (rc) return rc;
        }
    }

    *stmts_out = first;
    return 0;
}
