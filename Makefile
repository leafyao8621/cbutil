CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -g
SRC = $(wildcard src/*.c) $(wildcard src/*/*.c)
OBJ = $(SRC:.c=.o)
BIN = cbutil

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ -lcontainers -lriffutil

.PHONY: clean valgrind
clean:
	@rm -f $(OBJ)
	@rm -f $(BIN)
valgrind: $(BIN)
	@valgrind -s --leak-check=full --show-leak-kinds=all ./$(BIN)
