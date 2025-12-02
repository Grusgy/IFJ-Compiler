CC = gcc
CFLAGS = -Wall -Wextra -std=c11
FILES = ast.c generator.c parser.c scanner.c stack.c stmt.c symtable.c

.PHONY: ifjc clean

ifjc: $(FILES)
	$(CC) $(CFLAGS) -o $@ $(FILES)

clean:
	rm -f ifjc
