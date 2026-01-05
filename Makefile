.PHONY: all clean test help

# Variables
CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wstrict-prototypes
SRC = src/*
BIN = bin/
INCLUDE = -Iinclude

# Default target
all: $(BIN)program

$(BIN)program: $(SRC)
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^

test:
	@echo "Running tests..."

clean:
	rm -rf $(BIN)

help:
	@echo "Available targets:"
	@echo "  all    - Build the project"
	@echo "  test   - Run tests"
	@echo "  clean  - Remove build artifacts"