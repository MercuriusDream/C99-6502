CC=clang
CFLAGS=-std=c17 -O2 -Wall -Iinclude
SRC_DIR=src
INCLUDE_DIR=include
BIN_DIR=bin
TEST_DIR=tests

# Source files
SOURCES=$(wildcard $(SRC_DIR)/*.c)
MAIN_SRC=main.c
TEST_SRC=$(TEST_DIR)/verify_test.c

# Output binaries
TARGET=$(BIN_DIR)/mos6502
TEST_TARGET=$(BIN_DIR)/verify_test

# Default target
all: $(TARGET)

# Build main emulator
$(TARGET): $(SOURCES) $(MAIN_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SOURCES) $(MAIN_SRC) -o $(TARGET)
	@echo "Built $(TARGET)"

# Build test program
test: $(TEST_TARGET)

$(TEST_TARGET): $(SOURCES) $(TEST_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SOURCES) $(TEST_SRC) -o $(TEST_TARGET)
	@echo "Built $(TEST_TARGET)"

# Create bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR)
	@echo "Cleaned build artifacts"

# Run emulator with example ROM
run: $(TARGET)
	$(TARGET) -f examples/test.bin -a 8000

# Run with trace
run-trace: $(TARGET)
	$(TARGET) -f examples/test.bin -a 8000 -t

# Run verification tests
verify: $(TEST_TARGET)
	$(TEST_TARGET)

# Build example ROM
rom:
	cd examples && python3 build_test_rom.py

.PHONY: all test clean run run-trace verify rom