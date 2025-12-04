/*
    Implementace překladače imperativního jazyka IFJ25

    Marek Drábek (xdrabem00)
*/

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "generator.h"
#include "symtable.h"

// funkce ze scanneru pro nastavení vstupního proudu
extern void set_input_file(FILE *f);

int main(void) {
    // Nastavení vstupu podle zadání (standardní vstup)
    set_input_file(stdin);

    // Inicializace tabulky symbolů a ukazatele na hlavní AST
    SymTable global_symtable;
    symtable_init(&global_symtable);

    Stmt *program = NULL;

    // Spuštění parseru
    int parse_result = parse_program(&program, &global_symtable);

    // Zpracování návratového kódu (dle specifikace IFJ25)
    if (parse_result != 0) {
        switch (parse_result) {
            case 1:
                fprintf(stderr, "Lexikální chyba (kód 1)\n");
                break;
            case 2:
                fprintf(stderr, "Syntaktická chyba (kód 2)\n");
                break;
            case 3:
                fprintf(stderr, "Sémantická chyba (kód 3)\n");
                break;
            case 99:
                fprintf(stderr, "Interní chyba překladače (kód 99)\n");
                break;
            default:
                fprintf(stderr, "Neznámý kód chyby: %d\n", parse_result);
        }

        symtable_free(&global_symtable);
        return parse_result;
    }

    // Pokud vše proběhlo bez chyby → generujeme IFJcode25 na stdout
    codeGenerator(program);

    // Úklid (uvolnění symbolů)
    symtable_free(&global_symtable);

    // Návratová hodnota 0 = překlad proběhl úspěšně
    return 0;
}
