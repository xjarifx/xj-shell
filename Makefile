CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g
TARGET = customsh
SRC = src/main.c src/history.c src/parser.c src/builtins.c src/executor.c src/os_utils.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
