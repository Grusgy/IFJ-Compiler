#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "generator.h"
#include "stmt.h"
#include "symtable.h"

int main(int argc, char **argv) {
    // Pokud je zadán argument, přesměrujeme stdin z daného souboru
    if (argc == 2) {
        FILE *f = fopen(argv[1], "r");
        if (!f) {
            perror("Nemohu otevrit vstupni soubor");
            return 99; // interní chyba
        }
        // přesměrujeme stdin na tento soubor, protože scanner používá getchar()
        if (!freopen(argv[1], "r", stdin)) {
            perror("Nemohu presmerovat stdin");
            fclose(f);
            return 99;
        }
        // fclose(f);  // není nutné, stream je teď stdin
    } else if (argc > 2) {
        fprintf(stderr, "Pouziti: %s [soubor.ifj]\n", argv[0]);
        return 99;
    }

    Stmt *root = NULL;   // první příkaz programu (spojový seznam)
    SymTable symtable;   // globální tabulka symbolů

    // 1) Překlad vstupu: scanner + parser + sémantika
    int rc = parse_program(&root, &symtable);

    if (rc != 0) {
        // chyba – podle zadání IFJ se vrací přímo kód chyby
        switch (rc) {
            case 1:
                fprintf(stderr, "Lexikalni chyba (kod 1)\n");
                break;
            case 2:
                fprintf(stderr, "Syntakticka chyba (kod 2)\n");
                break;
            case 3:
                fprintf(stderr, "Semanticka chyba (kod 3)\n");
                break;
            default:
                fprintf(stderr, "Interni chyba (kod %d)\n", rc);
                rc = 99;   // jistota, že je to „interní chyba“
                break;
        }

        if (root) {
            stmt_free(root);
        }
        symtable_free(&symtable);

        return rc;
    }

    // 2) Pokud parse_program proběhl OK, vygenerujeme IFJcode25
    codeGenerator(root);

    // 3) Úklid paměti
    stmt_free(root);
    // symtable_free(&symtable);  // pokud časem uděláte

    return 0;
}
