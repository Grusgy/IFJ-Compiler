#include <stdlib.h>
#include <string.h>
#include "parser.h"

// =================== Globální stav ===================

// Globální aktuální token (plní ho scanner)
static Token cur_tok;

static int next_token(void) {
    return get_next_token(&cur_tok);
}

// =============== Deklarace interních funkcí ===============

static int parse_statement(Stmt **out_stmt, SymTable *symtable);
static int parse_expression(ast_t **out_expr);
static int parse_block(Stmt **out_block_stmt, SymTable *symtable);
static int parse_if(Stmt **out_stmt, SymTable *symtable);
static int parse_while(Stmt **out_stmt, SymTable *symtable);
static int parse_return_stmt(Stmt **out_stmt, SymTable *symtable);
static int parse_var_decl(Stmt **out_stmt, SymTable *symtable);
static int parse_fun_decl(Stmt **out_stmt, SymTable *symtable);
static int parse_id_statement(Stmt **out_stmt, SymTable *symtable);

// argumentové seznamy
static int build_param_list_for_ids(ast_t **params_out, size_t *count_out);
static int parse_expr_argument_list(ast_t **params_out, size_t *count_out);

// ================== Pomocné funkce ======================

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

static int is_builtin_func_token(TokenType t) {
    switch (t) {
        case TOK_FUNC_READ_STR:
        case TOK_FUNC_READ_NUM:
        case TOK_FUNC_WRITE:
        case TOK_FUNC_FLOOR:
        case TOK_FUNC_STR:
        case TOK_FUNC_LENGTH:
        case TOK_FUNC_SUBSTRING:
        case TOK_FUNC_STRCMP:
        case TOK_FUNC_ORD:
        case TOK_FUNC_CHR:
            return 1;
        default:
            return 0;
    }
}

// ================== PARSOVÁNÍ VÝRAZŮ ======================

// dopředu
static int parse_primary(ast_t **out);
static int parse_builtin_call(ast_t **out);

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

    // null literal
    if (cur_tok.type == TOK_NULL) {
        Token lit = cur_tok;
        int rc = next_token();
        if (rc) return rc;

        ast_t *node = ast_create_node(&lit, NULL, NULL);
        if (!node) return 99;
        *out = node;
        return 0;
    }

    // builtin funkce (Ifj.xxx(...)) jako výraz
    if (is_builtin_func_token(cur_tok.type)) {
        return parse_builtin_call(out);
    }

    // --- uživatelské funkce a identifikátory -----------------
    if (cur_tok.type == TOK_ID) {
        // uložíme si kopii tokenu se jménem
        Token id_tok = cur_tok;

        int rc = next_token();     // podíváme se, co je za id
        if (rc) return rc;

        // varianta 1: volání funkce id(expr, ...)
        if (cur_tok.type == TOK_LPAR) {
            rc = next_token();     // za '('
            if (rc) return rc;

            ast_t *params = NULL;
            size_t param_count = 0;

            if (cur_tok.type != TOK_RPAR) {
                // argumenty jsou výrazy
                rc = parse_expr_argument_list(&params, &param_count);
                if (rc) return rc;
            } else {
                // žádné argumenty → hlavičkový uzel s počtem 0
                Token header;
                header.type = TOK_CONST_INT;
                header.data.num_int_value = 0;
                params = ast_create_node(&header, NULL, NULL);
                if (!params) return 99;
            }

            if (cur_tok.type != TOK_RPAR) return 2;
            rc = next_token();     // za ')'
            if (rc) return rc;

            // kořen volání – token funkce, pravý podstrom = seznam parametrů
            ast_t *call_node = ast_create_node(&id_tok, NULL, params);
            if (!call_node) return 99;
            *out = call_node;
            return 0;
        }

        // varianta 2: jen proměnná (žádné závorky)
        ast_t *node = ast_create_node(&id_tok, NULL, NULL);
        if (!node) return 99;
        *out = node;
        return 0;
    }

    // GLOBAL_ID a literály
    if (cur_tok.type == TOK_GLOBAL_ID ||
        cur_tok.type == TOK_CONST_INT ||
        cur_tok.type == TOK_CONST_FLOAT ||
        cur_tok.type == TOK_CONST_STR ||
        cur_tok.type == TOK_CONST_ML_STR)
    {
        ast_t *node = ast_create_node(&cur_tok, NULL, NULL);
        if (!node) return 99;

        // vlastnictví případného str_value přechází na AST
        int rc = next_token();
        if (rc) return rc;

        *out = node;
        return 0;
    }

    return 2; // syntaktická chyba
}

static int parse_binop_rhs(int min_prec, ast_t *lhs, ast_t **out) {
    while (1) {
        int prec = get_precedence(cur_tok.type);
        if (prec < min_prec)
            break;

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

// builtin volání: FUNKCE ( expr, expr, ... )
static int parse_builtin_call(ast_t **out) {
    Token func_tok = cur_tok;   // Ifj.write / Ifj.read_num / ...

    int rc = next_token();      // za název funkce
    if (rc) return rc;

    if (cur_tok.type != TOK_LPAR) return 2;
    rc = next_token();          // za '('
    if (rc) return rc;

    ast_t *params = NULL;
    size_t param_count = 0;

    if (cur_tok.type != TOK_RPAR) {
        rc = parse_expr_argument_list(&params, &param_count);
        if (rc) return rc;
    } else {
        // prázdný seznam parametrů
        Token header;
        header.type = TOK_CONST_INT;
        header.data.num_int_value = 0;
        params = ast_create_node(&header, NULL, NULL);
        if (!params) return 99;
    }

    if (cur_tok.type != TOK_RPAR) return 2;
    rc = next_token();          // za ')'
    if (rc) return rc;

    // kořen volání – token funkce, pravý podstrom = seznam parametrů
    ast_t *call_node = ast_create_node(&func_tok, NULL, params);
    if (!call_node) return 99;
    *out = call_node;
    return 0;
}

// ================== SEZNAM ARGUMENTŮ ======================

// pro user funkce: id(id1, id2, ...)  -> hlavička (CONST_INT = count) + pravý řetěz ID uzlů
static int build_param_list_for_ids(ast_t **params_out, size_t *count_out) {
    ast_t *first = NULL;
    ast_t *last  = NULL;
    size_t count = 0;

    while (1) {
        if (cur_tok.type != TOK_ID && cur_tok.type != TOK_GLOBAL_ID)
            return 2;

        ast_t *param = ast_create_node(&cur_tok, NULL, NULL);
        if (!param) return 99;

        if (!first) first = param;
        else        last->right = param;
        last = param;
        count++;

        int rc = next_token();
        if (rc) return rc;

        if (cur_tok.type == TOK_COMMA) {
            rc = next_token();
            if (rc) return rc;
            continue;
        }
        break;
    }

    Token header;
    header.type = TOK_CONST_INT;
    header.data.num_int_value = (long long)count;

    ast_t *root = ast_create_node(&header, NULL, first);
    if (!root) return 99;

    *params_out = root;
    if (count_out) *count_out = count;
    return 0;
}

// pro builtin funkce: Ifj.write(expr, expr, ...)
static int parse_expr_argument_list(ast_t **params_out, size_t *count_out) {
    ast_t *first = NULL;
    ast_t *last  = NULL;
    size_t count = 0;

    // první argument
    ast_t *arg = NULL;
    int rc = parse_expression(&arg);
    if (rc) return rc;

    first = last = arg;
    count = 1;

    while (cur_tok.type == TOK_COMMA) {
        rc = next_token();
        if (rc) return rc;

        ast_t *next_arg = NULL;
        rc = parse_expression(&next_arg);
        if (rc) return rc;

        last->right = next_arg;
        last = next_arg;
        count++;
    }

    Token header;
    header.type = TOK_CONST_INT;
    header.data.num_int_value = (long long)count;

    ast_t *root = ast_create_node(&header, NULL, first);
    if (!root) return 99;

    *params_out = root;
    if (count_out) *count_out = count;
    return 0;
}

// ================== PARSOVÁNÍ PŘÍKAZŮ ======================

// var x [= expr]
static int parse_var_decl(Stmt **out_stmt, SymTable *symtable) {
    int rc = next_token();    // za "var"
    if (rc) return rc;

    if (cur_tok.type != TOK_ID) return 2;
    char *id_name = cur_tok.data.str_value;

    // sémantika: nesmí být redefinice
    if (symtable_find(symtable, id_name) != NULL) {
        return 3; // redeklarace
    }

    Stmt *stmt = stmt_create(STMT_VAR_DECL);
    if (!stmt) return 99;

    stmt->as.var_decl.var_name = id_name;   // vlastnictví stringu přechází na Stmt
    stmt->as.var_decl.expr = NULL;

    SymbolData sym = symbol_make_var(TYPE_UNDEF, false);
    if (!symtable_insert(symtable, id_name, sym)) return 99;

    rc = next_token();  // za ID
    if (rc) return rc;

    // volitelná inicializace: var x = expr
    if (cur_tok.type == TOK_ASSIGN) {
        rc = next_token(); // za '='
        if (rc) return rc;

        ast_t *expr = NULL;
        rc = parse_expression(&expr);
        if (rc) return rc;
        stmt->as.var_decl.expr = expr;

        SymbolData *found = symtable_find(symtable, stmt->as.var_decl.var_name);
        if (found && found->kind == SYM_VAR) {
            found->data.var.initialized = true;
        }
    }

    *out_stmt = stmt;
    return 0;
}

// id ... -> přiřazení nebo volání funkce (uživatelské)
static int parse_id_statement(Stmt **out_stmt, SymTable *symtable) {
    if (cur_tok.type != TOK_ID && cur_tok.type != TOK_GLOBAL_ID) return 2;

    char *id_name = cur_tok.data.str_value;
    int is_global = (cur_tok.type == TOK_GLOBAL_ID);

    int rc = next_token(); // za ID / GLOBAL_ID
    if (rc) return rc;

    // varianta 1: přiřazení
    if (cur_tok.type == TOK_ASSIGN) {
        SymbolData *found = symtable_find(symtable, id_name);

        // lokální ID musí existovat
        if ((!found || found->kind != SYM_VAR) && !is_global) {
            return 3; // použití nedefinované proměnné
        }

        // GLOBAL_ID (typicky "__d") – pokud neexistuje, vytvoříme dummy var
        if ((!found || found->kind != SYM_VAR) && is_global) {
            SymbolData sym = symbol_make_var(TYPE_UNDEF, true);
            if (!symtable_insert(symtable, id_name, sym)) return 99;
            found = symtable_find(symtable, id_name);
        }

        Stmt *stmt = stmt_create(STMT_ASSIGN);
        if (!stmt) return 99;

        stmt->as.assign.var_name = id_name;
        stmt->as.assign.expr = NULL;

        rc = next_token(); // za '='
        if (rc) return rc;

        ast_t *expr = NULL;
        rc = parse_expression(&expr);
        if (rc) return rc;

        stmt->as.assign.expr = expr;

        if (found && found->kind == SYM_VAR) {
            found->data.var.initialized = true;
        }

        *out_stmt = stmt;
        return 0;
    }

    // varianta 2: volání uživatelské funkce id(...)
    if (cur_tok.type == TOK_LPAR) {
        Stmt *stmt = stmt_create(STMT_FUN_CALL);
        if (!stmt) return 99;

        stmt->as.fun_call.fun_name = id_name;
        stmt->as.fun_call.parameters = NULL;

        rc = next_token(); // za '('
        if (rc) return rc;

        ast_t *params = NULL;
        size_t dummy_count = 0;

        if (cur_tok.type != TOK_RPAR) {
            rc = build_param_list_for_ids(&params, &dummy_count);
            if (rc) return rc;
        }

        if (cur_tok.type != TOK_RPAR) return 2;
        rc = next_token(); // za ')'
        if (rc) return rc;

        stmt->as.fun_call.parameters = params;
        *out_stmt = stmt;
        return 0;
    }

    return 2;
}

// while (cond) { ... }
static int parse_while(Stmt **out_stmt, SymTable *symtable) {
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

    rc = parse_block(&stmt->as.while_loop.block, symtable);
    if (rc) return rc;

    *out_stmt = stmt;
    return 0;
}

// return [id | __global]
static int parse_return_stmt(Stmt **out_stmt, SymTable *symtable) {
    (void)symtable; // zatím nepoužité

    Stmt *stmt = stmt_create(STMT_RETURN);
    if (!stmt) return 99;
    stmt->as.return_.var_name = NULL;

    int rc = next_token(); // za "return"
    if (rc) return rc;

    // return bez hodnoty (void)
    if (cur_tok.type == TOK_EOL ||
        cur_tok.type == TOK_RBRACE ||
        cur_tok.type == TOK_EOF)
    {
        *out_stmt = stmt;
        return 0;
    }

    if (cur_tok.type == TOK_ID || cur_tok.type == TOK_GLOBAL_ID) {
        char *id_name = cur_tok.data.str_value;
        stmt->as.return_.var_name = id_name;
        rc = next_token();
        if (rc) return rc;
        *out_stmt = stmt;
        return 0;
    }

    return 2;
}

// { stmt* }
static int parse_block(Stmt **out_block_stmt, SymTable *symtable) {
    if (cur_tok.type != TOK_LBRACE) return 2;

    int rc = next_token();   // za '{'
    if (rc) return rc;

    Stmt *first = NULL;
    Stmt *last = NULL;

    while (cur_tok.type != TOK_RBRACE && cur_tok.type != TOK_EOF) {
        while (cur_tok.type == TOK_EOL) {
            rc = next_token();
            if (rc) return rc;
        }
        if (cur_tok.type == TOK_RBRACE || cur_tok.type == TOK_EOF) break;

        Stmt *stmt = NULL;
        rc = parse_statement(&stmt, symtable);
        if (rc) return rc;
        if (!stmt) break;

        if (!first) first = stmt;
        else        last->next = stmt;

        while (stmt->next) stmt = stmt->next;
        last = stmt;

        if (cur_tok.type == TOK_EOL) {
            rc = next_token();
            if (rc) return rc;
        }
    }

    if (cur_tok.type != TOK_RBRACE) return 2;

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

    rc = parse_block(&stmt->as.if_stmt.then_branch, symtable);
    if (rc) return rc;

    if (cur_tok.type == TOK_ELSE) {
        rc = next_token();  // za "else"
        if (rc) return rc;
        rc = parse_block(&stmt->as.if_stmt.else_branch, symtable);
        if (rc) return rc;
    }

    *out_stmt = stmt;
    return 0;
}

// jeden příkaz v těle funkce / bloku
static int parse_statement(Stmt **out_stmt, SymTable *symtable) {
    *out_stmt = NULL;

    switch (cur_tok.type) {
        case TOK_VAR:
            return parse_var_decl(out_stmt, symtable);

        case TOK_ID:
        case TOK_GLOBAL_ID:
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
            return 0;

        default:
            return 2;
    }
}

// ================== Funkce (static main, static factorial...) ==================

// static name(param1, param2) { body }
static int parse_fun_decl(Stmt **out_stmt, SymTable *symtable) {
    int rc = next_token();  // za "static"
    if (rc) return rc;

    if (cur_tok.type != TOK_ID) return 2;
    char *fun_name = cur_tok.data.str_value;

    rc = next_token();      // za jméno funkce
    if (rc) return rc;

    if (cur_tok.type != TOK_LPAR) return 2;
    rc = next_token();      // za '('
    if (rc) return rc;

    ast_t *params_ast = NULL;
    size_t param_count = 0;

    if (cur_tok.type != TOK_RPAR) {
        rc = build_param_list_for_ids(&params_ast, &param_count);
        if (rc) return rc;
    }

    if (cur_tok.type != TOK_RPAR) return 2;
    rc = next_token();      // za ')'
    if (rc) return rc;

    // přeskočit prázdné řádky před '{'
    while (cur_tok.type == TOK_EOL) {
        rc = next_token();
        if (rc) return rc;
    }

    Stmt *body_block = NULL;
    rc = parse_block(&body_block, symtable);
    if (rc) return rc;

    // vložíme funkci do tabulky symbolů
    SymbolData f = symbol_make_func(fun_name, TYPE_UNDEF, param_count);
    if (!symtable_insert(symtable, fun_name, f)) return 99;

    Stmt *fun_stmt = stmt_create(STMT_FUN_DECL);
    if (!fun_stmt) return 99;

    fun_stmt->as.fun_dec.fun_name   = fun_name;
    fun_stmt->as.fun_dec.parameters = params_ast;
    fun_stmt->as.fun_dec.fun_block  = body_block;

    *out_stmt = fun_stmt;
    return 0;
}

// ================== HLAVNÍ FUNKCE PARSERU ==================

// Program:
//  [import "ifj25" for Ifj]
//  class Program {
//      static main() { ... }
//      static factorial(n) { ... }
//      ...
//  }
// ================== HLAVNÍ FUNKCE PARSERU ==================

// Program (zjednodušeně):
//   [nějaké věci před class - prolog, komentáře, EOL...]
//   class Program {
//       static main(...) { ... }
//       static factorial(...) { ... }
//       ...
//   }
int parse_program(Stmt **stmts_out, SymTable *symtable) {
    *stmts_out = NULL;
    symtable_init(symtable);

    int rc = next_token();
    if (rc) return rc;

    // 1) Pro jistotu: přeskočíme úplně VŠECHNO, dokud nenarazíme na 'class'
    //    (TOK_CLASS), nebo EOF.
    while (cur_tok.type != TOK_CLASS && cur_tok.type != TOK_EOF) {
        rc = next_token();
        if (rc) return rc;
    }

    if (cur_tok.type != TOK_CLASS) {
        // nedošli jsme ke 'class' -> syntaktická chyba programu
        return 2;
    }

    // 2) class Program
    rc = next_token();       // za 'class'
    if (rc) return rc;

    if (cur_tok.type != TOK_ID) return 2;  // název třídy (Program)
    // jméno třídy teď neukládáme do symtable
    rc = next_token();
    if (rc) return rc;

    // přeskočíme prázdné řádky
    while (cur_tok.type == TOK_EOL) {
        rc = next_token();
        if (rc) return rc;
    }

    if (cur_tok.type != TOK_LBRACE) return 2;
    rc = next_token();  // za '{'
    if (rc) return rc;

    Stmt *first = NULL;
    Stmt *last  = NULL;

    // 3) tělo třídy – očekáváme jen 'static' funkce, plus EOL
    while (cur_tok.type != TOK_RBRACE && cur_tok.type != TOK_EOF) {
        // přeskočíme prázdné řádky
        while (cur_tok.type == TOK_EOL) {
            rc = next_token();
            if (rc) return rc;
        }
        if (cur_tok.type == TOK_RBRACE || cur_tok.type == TOK_EOF) break;

        if (cur_tok.type != TOK_STATIC) {
            // uvnitř třídy něco jiného než 'static' => zatím neznáme, bereme jako chybu
            return 2;
        }

        Stmt *fun_stmt = NULL;
        rc = parse_fun_decl(&fun_stmt, symtable);
        if (rc) return rc;

        if (!first) first = fun_stmt;
        else        last->next = fun_stmt;

        while (fun_stmt->next) fun_stmt = fun_stmt->next;
        last = fun_stmt;
    }

    if (cur_tok.type != TOK_RBRACE) return 2;
    rc = next_token();    // za '}'
    if (rc) return rc;

    // přeskočíme případné EOL na konci souboru
    while (cur_tok.type == TOK_EOL) {
        rc = next_token();
        if (rc) return rc;
    }

    // za třídou už nechceme vidět nic jiného než EOF
    if (cur_tok.type != TOK_EOF) return 2;

    *stmts_out = first;
    return 0;
}

