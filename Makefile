CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
INCLUDES = -Iinclude
SRC = src/main.c src/parser.c src/builtins.c src/history.c
TARGET = xjsh

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
