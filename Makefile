CC = gcc
CFLAGS = -Wall -Wextra -std=c11

SRC = ast.c generator.c parser.c scanner.c stack.c stmt.c symtable.c
OBJ = $(SRC:.c=.o)
TARGET = ifjc

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ) $(TARGET)
