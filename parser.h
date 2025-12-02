#ifndef IFJ_COMPILER_PARSER_H
#define IFJ_COMPILER_PARSER_H

#include "scanner.h"
#include "ast.h"
#include "stmt.h"
#include "symtable.h"

// Hlavní funkce parseru.
// - stmts_out: výstup – první příkaz programu (spojový seznam přes Stmt->next)
// - symtable: globální tabulka symbolů (bude naplněná deklaracemi)
//
// Návratové kódy:
//  0  = OK
//  1  = lexikální chyba
//  2  = syntaktická chyba
//  3  = sémantická chyba
// 99  = interní chyba (malloc apod.)
int parse_program(Stmt **stmts_out, SymTable *symtable);

#endif // IFJ_COMPILER_PARSER_H
