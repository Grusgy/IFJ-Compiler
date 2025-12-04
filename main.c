/*
    Implementace překladače imperativního jazyka IFJ25

    Marek Drábek:   xdrabem00
*/

#include <stdio.h>
#include "parser.h"
#include "generator.h"
#include "symtable.h"

int main(void) {
    SymTable global_symtable;
    symtable_init(&global_symtable);

    Stmt *program = NULL;

    // Spuštění parseru – čte ze stdin přes get_next_token()
    int parse_result = parse_program(&program, &global_symtable);

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

    // Generování IFJcode25 na stdout
    codeGenerator(program);

    symtable_free(&global_symtable);
    return 0;
}
