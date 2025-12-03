# Kompilátor a flagy (přesně podle zadání)
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g

# Všechny zdrojové soubory
SRC = main.c ast.c generator.c parser.c scanner.c stack.c stmt.c symtable.c

# Výsledný binární soubor
TARGET = ifj25

# Překlad projektu
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Vyčištění projektu
clean:
	rm -f $(TARGET)
