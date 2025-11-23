CC=clang
CFLAGS=-std=c99 -O2 -Wall -Iinclude
SRC_DIR=src
INCLUDE_DIR=include
BIN_DIR=bin
TEST_DIR=tests

# Core sources (C99 compliant, no POSIX dependencies)
CORE_SOURCES=$(filter-out $(SRC_DIR)/tui_monitor.c $(SRC_DIR)/debugger.c, $(wildcard $(SRC_DIR)/*.c))

# POSIX sources (includes monitor and debugger)
POSIX_SOURCES=$(wildcard $(SRC_DIR)/*.c)

MAIN_SRC=main.c
TEST_SRC=$(TEST_DIR)/minimal/verify_test.c
FUNCTIONAL_TEST_SRC=$(TEST_DIR)/6502_functional_test/run_functional_test.c
DEBUG_TEST_SRC=$(TEST_DIR)/debug_test.c
INTERRUPT_TEST_SRC=$(TEST_DIR)/interrupt_test.c

# Output binaries
TARGET_MAIN=$(BIN_DIR)/mos6502_main
TARGET_POSIX=$(BIN_DIR)/mos6502
TEST_TARGET=$(BIN_DIR)/verify_test
FUNCTIONAL_TEST_TARGET=$(BIN_DIR)/functional_test
DEBUG_TEST_TARGET=$(BIN_DIR)/debug_test
INTERRUPT_TEST_TARGET=$(BIN_DIR)/interrupt_test
TEST_65C02_TARGET=$(BIN_DIR)/test_65c02
VERIFY_65C02_TARGET=$(BIN_DIR)/verify_65c02_test

# Default target: POSIX-compliant build with TUI monitor
all: posix

# POSIX build (with TUI monitor support)
posix: $(TARGET_POSIX)

$(TARGET_POSIX): $(POSIX_SOURCES) $(MAIN_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(POSIX_SOURCES) $(MAIN_SRC) -lncurses -o $(TARGET_POSIX)
	@echo "Built $(TARGET_POSIX) (POSIX mode with TUI monitor support)"

# Pure C99 build (no POSIX dependencies)
main: $(TARGET_MAIN)

$(TARGET_MAIN): $(CORE_SOURCES) $(MAIN_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) -DNO_TUI_MONITOR $(CORE_SOURCES) $(MAIN_SRC) -o $(TARGET_MAIN)
	@echo "Built $(TARGET_MAIN) (pure C99 mode without TUI monitor)"

# Help target
help:
	@echo "C99-6502 Emulator Build System"
	@echo ""
	@echo "Build Targets:"
	@echo "  make / make all / make posix  - Build POSIX-compliant emulator with TUI monitor (default)"
	@echo "  make main                     - Build pure C99-compliant emulator (no POSIX)"
	@echo ""
	@echo "Run Targets:"
	@echo "  make run                - Run POSIX emulator with example ROM"
	@echo "  make run-main           - Run C99 emulator with example ROM"
	@echo "  make run-trace          - Run with execution trace"
	@echo "  make mon / make monitor - Build and run with TUI monitor"
	@echo ""
	@echo "Test Targets:"
	@echo "  make test               - Build verification test"
	@echo "  make verify             - Build and run verification test"
	@echo "  make functional-test    - Build Klaus Dormann functional test"
	@echo "  make debug-test         - Build debugger test"
	@echo "  make interrupt-test     - Build interrupt test"
	@echo "  make verify-65c02       - Build 65C02 verification test"
	@echo ""
	@echo "Utility Targets:"
	@echo "  make clean              - Remove all build artifacts"
	@echo "  make rom                - Build example ROM"

# Build test program (uses core sources only)
test: $(TEST_TARGET)

$(TEST_TARGET): $(CORE_SOURCES) $(TEST_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(CORE_SOURCES) $(TEST_SRC) -o $(TEST_TARGET)
	@echo "Built $(TEST_TARGET)"

# Build functional test runner
functional-test: $(FUNCTIONAL_TEST_TARGET)

$(FUNCTIONAL_TEST_TARGET): $(CORE_SOURCES) $(FUNCTIONAL_TEST_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(CORE_SOURCES) $(FUNCTIONAL_TEST_SRC) -o $(FUNCTIONAL_TEST_TARGET)
	@echo "Built $(FUNCTIONAL_TEST_TARGET)"

# Build debug test
debug-test: $(DEBUG_TEST_TARGET)

$(DEBUG_TEST_TARGET): $(CORE_SOURCES) $(DEBUG_TEST_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(CORE_SOURCES) $(DEBUG_TEST_SRC) -o $(DEBUG_TEST_TARGET)
	@echo "Built $(DEBUG_TEST_TARGET)"

# Build interrupt test
interrupt-test: $(INTERRUPT_TEST_TARGET)

$(INTERRUPT_TEST_TARGET): $(CORE_SOURCES) $(INTERRUPT_TEST_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(CORE_SOURCES) $(INTERRUPT_TEST_SRC) -o $(INTERRUPT_TEST_TARGET)
	@echo "Built $(INTERRUPT_TEST_TARGET)"

# Build 65C02 extended test
test-65c02: $(TEST_65C02_TARGET)

$(TEST_65C02_TARGET): $(CORE_SOURCES) $(TEST_DIR)/6502_functional_test/run_65c02_test.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(CORE_SOURCES) $(TEST_DIR)/6502_functional_test/run_65c02_test.c -o $(TEST_65C02_TARGET)
	@echo "Built $(TEST_65C02_TARGET)"

# Build 65C02 verification test
verify-65c02: $(VERIFY_65C02_TARGET)

$(VERIFY_65C02_TARGET): $(CORE_SOURCES) $(TEST_DIR)/minimal/verify_65c02_test.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(CORE_SOURCES) $(TEST_DIR)/minimal/verify_65c02_test.c -o $(VERIFY_65C02_TARGET)
	@echo "Built $(VERIFY_65C02_TARGET)"

# Create bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR)
	@echo "Cleaned build artifacts"

# Run POSIX emulator with example ROM
run: $(TARGET_POSIX)
	$(TARGET_POSIX) -f tests/minimal/test.bin -a 8000

# Run C99 emulator with example ROM
run-main: $(TARGET_MAIN)
	$(TARGET_MAIN) -f tests/minimal/test.bin -a 8000

# Run with trace
run-trace: $(TARGET_POSIX)
	$(TARGET_POSIX) -f tests/minimal/test.bin -a 8000 -t

# Run with TUI monitor (build and run)
mon: $(TARGET_POSIX)
	$(TARGET_POSIX) -m

monitor: mon

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

# Run 65C02 extended test
run-test-65c02: $(TEST_65C02_TARGET)
	$(TEST_65C02_TARGET)

# Run 65C02 verification test
run-verify-65c02: $(VERIFY_65C02_TARGET)
	$(VERIFY_65C02_TARGET)

# Run reset test
run-reset-test: reset-test
	$(BIN_DIR)/verify_reset_test

reset-test: $(CORE_SOURCES) tests/minimal/verify_reset.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(CORE_SOURCES) tests/minimal/verify_reset.c -o $(BIN_DIR)/verify_reset_test
	@echo "Built Reset Test"

.PHONY: all posix main help test clean run run-main run-trace mon monitor verify rom functional-test run-functional-test download-functional-test debug-test run-debug-test interrupt-test run-interrupt-test test-65c02 run-test-65c02 verify-65c02 run-verify-65c02 run-reset-test