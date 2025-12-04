/*
    Implementace překladače imperativního jazyka IFJ25

    Marek Drábek:   xdrabem00
*/

#include "symtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ===========================================================
// Interní struktura uzlu AVL stromu
// ===========================================================
typedef struct SymNode {
    char *key;               // jméno symbolu
    SymbolData data;         // obsah (proměnná, funkce, ...)
    struct SymNode *left;    // levý podstrom
    struct SymNode *right;   // pravý podstrom
    int height;              // výška podstromu (pro AVL)
} SymNode;

// ===========================================================
// Pomocné AVL funkce
// ===========================================================
static int height(SymNode *n) { return n ? n->height : 0; }
static int max(int a, int b) { return a > b ? a : b; }

static SymNode *rotate_right(SymNode *y) {
    SymNode *x = y->left;
    SymNode *T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;
    return x;
}

static SymNode *rotate_left(SymNode *x) {
    SymNode *y = x->right;
    SymNode *T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;
    return y;
}

static int get_balance(SymNode *n) {
    return n ? height(n->left) - height(n->right) : 0;
}

static char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (!p) {
        return NULL;
    }
    memcpy(p, s, len);   // zkopíruje i '\0'
    return p;
}


// ===========================================================
// Vytvoření nového uzlu
// ===========================================================
static SymNode *new_node(const char *key, SymbolData data) {
    SymNode *n = malloc(sizeof(SymNode));
    if (!n) return NULL;

    n->key = my_strdup(key);
    if (!n->key) {
        free(n);
        return NULL;
    }

    n->data = data;
    n->left = n->right = NULL;
    n->height = 1;
    return n;
}

// ===========================================================
// Rekurzivní insert do AVL
// ===========================================================
static SymNode *insert_node(SymNode *node, const char *key, SymbolData data, bool *alloc_ok) {
    if (!node) {
        SymNode *n = new_node(key, data);
        if (!n) *alloc_ok = false;
        return n;
    }

    int cmp = strcmp(key, node->key);
    if (cmp < 0) {
        node->left = insert_node(node->left, key, data, alloc_ok);
    } else if (cmp > 0) {
        node->right = insert_node(node->right, key, data, alloc_ok);
    } else {
        // symbol už existuje – přepíšeme data
        node->data = data;
        return node;
    }

    node->height = 1 + max(height(node->left), height(node->right));
    int balance = get_balance(node);

    // LL
    if (balance > 1 && strcmp(key, node->left->key) < 0)
        return rotate_right(node);

    // RR
    if (balance < -1 && strcmp(key, node->right->key) > 0)
        return rotate_left(node);

    // LR
    if (balance > 1 && strcmp(key, node->left->key) > 0) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    // RL
    if (balance < -1 && strcmp(key, node->right->key) < 0) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

// ===========================================================
// Vyhledání uzlu podle klíče (jen v jednom stromu)
// ===========================================================
static SymNode *find_node(SymNode *node, const char *key) {
    if (!node) return NULL;
    int cmp = strcmp(key, node->key);
    if (cmp == 0) return node;
    if (cmp < 0) return find_node(node->left, key);
    return find_node(node->right, key);
}

// ===========================================================
// Uvolnění všech uzlů jednoho stromu
// ===========================================================
static void free_node(SymNode *node) {
    if (!node) return;
    free_node(node->left);
    free_node(node->right);

    if (node->data.kind == SYM_FUNC) {
        free(node->data.data.func.name);
        free(node->data.data.func.params);
    }

    free(node->key);
    free(node);
}

void symtable_init(SymTable *table) {
    table->root = NULL;
    table->parent = NULL;
}

// nový scope s daným rodičem
void symtable_init_child(SymTable *child, SymTable *parent) {
    child->root = NULL;
    child->parent = parent;
}

void symtable_free(SymTable *table) {
    free_node(table->root);
    table->root = NULL;
    // parent NEuvolňujeme, ten vlastní někdo jiný
}

// vloží symbol pouze do aktuální tabulky
bool symtable_insert_here(SymTable *table, const char *key, SymbolData data) {
    bool ok = true;
    table->root = insert_node(table->root, key, data, &ok);
    return ok;
}

// zpětně kompatibilní alias
bool symtable_insert(SymTable *table, const char *key, SymbolData data) {
    return symtable_insert_here(table, key, data);
}

// hledá jen v aktuální tabulce
SymbolData *symtable_find_here(SymTable *table, const char *key) {
    SymNode *n = find_node(table->root, key);
    return n ? &n->data : NULL;
}

// hledá v aktuální tabulce a všech rodičích
SymbolData *symtable_find(SymTable *table, const char *key) {
    SymTable *cur = table;
    while (cur) {
        SymNode *n = find_node(cur->root, key);
        if (n) return &n->data;
        cur = cur->parent;
    }
    return NULL;
}

bool symtable_delete(SymTable *table, const char *key) {
    (void)table;
    (void)key;
    return false;
}

// ===========================================================
// Helpery pro snadné vytvoření symbolů
// ===========================================================

SymbolData symbol_make_var(DataType type, bool initialized) {
    SymbolData s;
    s.kind = SYM_VAR;
    s.data.var.type = type;
    s.data.var.initialized = initialized;
    s.data.var.is_const = false;
    return s;
}

SymbolData symbol_make_func(const char *name, DataType return_type, size_t param_count) {
    SymbolData s;
    s.kind = SYM_FUNC;
    s.data.func.name = my_strdup(name);
    s.data.func.return_type = return_type;
    s.data.func.param_count = param_count;
    s.data.func.params = NULL;
    s.data.func.defined = false;
    return s;
}

// ===========================================================
// Debug výpis (pomocná funkce)
// ===========================================================
static void print_node(SymNode *node, int depth) {
    if (!node) return;
    print_node(node->right, depth + 1);
    for (int i = 0; i < depth; ++i) printf("   ");
    printf("%s [%s]\n", node->key,
           node->data.kind == SYM_VAR ? "VAR" :
           node->data.kind == SYM_FUNC ? "FUNC" : "OTHER");
    print_node(node->left, depth + 1);
}

void symtable_debug_print(SymTable *table) {
    printf("=== SYMBOL TABLE ===\n");
    print_node(table->root, 0);
    printf("====================\n");
}
