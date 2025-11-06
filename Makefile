CC=clang
CFLAGS=-std=c17 -O2 -Wall -Iinclude
SRC_DIR=src
INCLUDE_DIR=include
BIN_DIR=bin
TEST_DIR=tests

# Source files
SOURCES=$(wildcard $(SRC_DIR)/*.c)
MAIN_SRC=main.c
TEST_SRC=$(TEST_DIR)/minimal/verify_test.c
FUNCTIONAL_TEST_SRC=$(TEST_DIR)/6502_functional_test/run_functional_test.c
DEBUG_TEST_SRC=$(TEST_DIR)/debug_test.c
INTERRUPT_TEST_SRC=$(TEST_DIR)/interrupt_test.c

# Output binaries
TARGET=$(BIN_DIR)/mos6502
TEST_TARGET=$(BIN_DIR)/verify_test
FUNCTIONAL_TEST_TARGET=$(BIN_DIR)/functional_test
DEBUG_TEST_TARGET=$(BIN_DIR)/debug_test
INTERRUPT_TEST_TARGET=$(BIN_DIR)/interrupt_test

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

# Build functional test runner
functional-test: $(FUNCTIONAL_TEST_TARGET)

$(FUNCTIONAL_TEST_TARGET): $(SOURCES) $(FUNCTIONAL_TEST_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SOURCES) $(FUNCTIONAL_TEST_SRC) -o $(FUNCTIONAL_TEST_TARGET)
	@echo "Built $(FUNCTIONAL_TEST_TARGET)"

# Build debug test
debug-test: $(DEBUG_TEST_TARGET)

$(DEBUG_TEST_TARGET): $(SOURCES) $(DEBUG_TEST_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SOURCES) $(DEBUG_TEST_SRC) -o $(DEBUG_TEST_TARGET)
	@echo "Built $(DEBUG_TEST_TARGET)"

# Build interrupt test
interrupt-test: $(INTERRUPT_TEST_TARGET)

$(INTERRUPT_TEST_TARGET): $(SOURCES) $(INTERRUPT_TEST_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SOURCES) $(INTERRUPT_TEST_SRC) -o $(INTERRUPT_TEST_TARGET)
	@echo "Built $(INTERRUPT_TEST_TARGET)"

# Create bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR)
	@echo "Cleaned build artifacts"

# Run emulator with example ROM
run: $(TARGET)
	$(TARGET) -f tests/minimal/test.bin -a 8000

# Run with trace
run-trace: $(TARGET)
	$(TARGET) -f tests/minimal/test.bin -a 8000 -t

# Run verification tests
verify: $(TEST_TARGET)
	$(TEST_TARGET)

# Run Klaus Dormann functional tests
run-functional-test: $(FUNCTIONAL_TEST_TARGET)
	$(FUNCTIONAL_TEST_TARGET)

# Download Klaus Dormann test ROM if not present
download-functional-test:
	@mkdir -p tests/6502_functional_test
	@if [ ! -f tests/6502_functional_test/6502_functional_test.bin ]; then \
		echo "Downloading Klaus Dormann functional test..."; \
		curl -L -o tests/6502_functional_test/6502_functional_test.bin https://raw.githubusercontent.com/Klaus2m5/6502_65C02_functional_tests/master/bin_files/6502_functional_test.bin; \
		curl -L -o tests/6502_functional_test/6502_functional_test.lst https://raw.githubusercontent.com/Klaus2m5/6502_65C02_functional_tests/master/bin_files/6502_functional_test.lst; \
		echo "Download complete."; \
	else \
		echo "Functional test already downloaded."; \
	fi

# Build example ROM
rom:
	cd tests/minimal && python3 build_test_rom.py

# Run debug test
run-debug-test: $(DEBUG_TEST_TARGET)
	$(DEBUG_TEST_TARGET)

# Run interrupt test
run-interrupt-test: $(INTERRUPT_TEST_TARGET)
	$(INTERRUPT_TEST_TARGET)

.PHONY: all test clean run run-trace verify rom functional-test run-functional-test download-functional-test debug-test run-debug-test interrupt-test run-interrupt-test