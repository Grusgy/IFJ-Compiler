#ifndef IFJ_COMPILER_SYMTABLE_H
#define IFJ_COMPILER_SYMTABLE_H

#include <stdbool.h>
#include <stddef.h>

// ===========================================================
// Druh symbolu – o jaký typ identifikátoru se jedná
// ===========================================================
typedef enum {
    SYM_VAR,      // proměnná (včetně parametrů)
    SYM_FUNC,     // funkce
    SYM_CLASS,    // třída (připraveno pro rozšíření)
    SYM_UNDEFINED // pro dočasné nebo chybové stavy
} SymbolKind;

// ===========================================================
// Typy dat podle jazyka IFJ25
// ===========================================================
typedef enum {
    TYPE_UNDEF,   // neznámý / zatím neurčený
    TYPE_INT,     // Num
    TYPE_FLOAT,   // desetinné číslo
    TYPE_STRING,  // String
    TYPE_BOOL,    // true/false
    TYPE_NULL     // null
} DataType;

// ===========================================================
// Parametr funkce
// ===========================================================
typedef struct {
    char *name;       // jméno parametru
    DataType type;    // typ parametru
} FunctionParam;

// ===========================================================
// Informace o funkci
// ===========================================================
typedef struct {
    char *name;              // název funkce
    DataType return_type;    // návratový typ
    size_t param_count;      // počet parametrů
    FunctionParam *params;   // dynamické pole parametrů
    bool defined;            // true, pokud byla funkce definována (ne jen deklarována)
} FunctionInfo;

// ===========================================================
// Informace o proměnné
// ===========================================================
typedef struct {
    DataType type;           // datový typ (Num, String, atd.)
    bool initialized;        // jestli už má hodnotu
    bool is_const;           // pro případné rozšíření o konstanty
} VariableInfo;

// ===========================================================
// Obecná položka symbolu – ukládaná v AVL stromu
// ===========================================================
typedef struct {
    SymbolKind kind;         // SYM_VAR, SYM_FUNC, ...
    union {
        VariableInfo var;    // data o proměnné
        FunctionInfo func;   // data o funkci
    } data;
} SymbolData;

// ===========================================================
// Struktura celé tabulky symbolů (AVL strom)
// ===========================================================
typedef struct symTable_t {
    struct SymNode *root;    // kořen AVL stromu
} SymTable;

// ===========================================================
// Veřejné API tabulky symbolů
// ===========================================================

// Inicializace tabulky
void symtable_init(SymTable *table);

// Uvolnění tabulky a všech uzlů
void symtable_free(SymTable *table);

// Vložení nového symbolu (návrat false = chyba alokace)
bool symtable_insert(SymTable *table, const char *key, SymbolData data);

// Najde symbol podle jména, NULL pokud neexistuje
SymbolData *symtable_find(SymTable *table, const char *key);

// Smazání konkrétního symbolu (volitelné)
bool symtable_delete(SymTable *table, const char *key);

// Pomocné funkce pro tvorbu symbolů
SymbolData symbol_make_var(DataType type, bool initialized);
SymbolData symbol_make_func(const char *name, DataType return_type, size_t param_count);

// Výpis pro debug
void symtable_debug_print(SymTable *table);

#endif // IFJ_COMPILER_SYMTABLE_H
