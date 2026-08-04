CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -Iinclude
SRC = src/main.c src/dictionary.c src/file_manager.c src/input.c src/statistics.c
TARGET = lexicore

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean