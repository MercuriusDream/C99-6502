#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "instruments_table.h"
#include "instruments_implementation.h"
#include "memory.h"
#include "loader.h"
#include "trace.h"

#define TEST_START_ADDRESS 0x0400
#define TEST_SUCCESS_ADDRESS 0x3469
#define MAX_CYCLES 100000000  // 100 million cycles should be enough

int main(int argc, char** argv) {
    const char* test_file = "tests/6502_functional_test/6502_functional_test.bin";
    CPU_VARIANT cpu_variant = CPU_VARIANT_NMOS_6502;  // Default to NMOS
    int enable_trace = 0;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0) {
            enable_trace = 1;
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            test_file = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cpu") == 0) {
            if (i + 1 < argc) {
                i++;
                if (strcmp(argv[i], "6502") == 0 || strcmp(argv[i], "nmos") == 0) {
                    cpu_variant = CPU_VARIANT_NMOS_6502;
                } else if (strcmp(argv[i], "65c02") == 0 || strcmp(argv[i], "cmos") == 0) {
                    cpu_variant = CPU_VARIANT_CMOS_65C02;
                } else {
                    printf("Unknown CPU variant: %s\n", argv[i]);
                    return 1;
                }
            }
        }
    }

    // Set CPU variant
    cpu_set_variant(cpu_variant);

    printf("Klaus Dormann 6502 Functional Test");
    printf("CPU Variant: %s\n",
           cpu_variant == CPU_VARIANT_CMOS_65C02 ? "CMOS 65C02" : "NMOS 6502");
    printf("Test file: %s\n\n", test_file);

    // Setup memory - entire 64KB as RAM for the test
    // Note: Can't use 0x10000 as size (wraps to 0 in 16-bit), so use 0xFFFF + special handling
    // Or split into two 32KB regions
    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);  // Lower 32KB RAM
    mem_region_add_ram(0x8000, 0x8000);  // Upper 32KB RAM
    printf("Memory configured: $0000-$FFFF (64KB RAM)\n");

    // Load the test binary at $0000
    if (load_bin_region(test_file, 0x0000) != 0) {
        printf("Failed to load test binary from: %s\n", test_file);
        return 1;
    }
    printf("Test binary loaded at $0000\n");

    // Initialize bus
    mem_region_init();

    // Initialize CPU and set PC to test start address
    cpu_reset();
    REG.PC = TEST_START_ADDRESS;
    printf("PC set to $%04X (test start)\n", TEST_START_ADDRESS);
    printf("Success target: PC = $%04X\n\n", TEST_SUCCESS_ADDRESS);

    // Track PC to detect infinite loops - use a small history buffer
    #define HISTORY_SIZE 1000
    MEM_TWO_WORDS pc_history[HISTORY_SIZE];
    unsigned int history_count[HISTORY_SIZE];
    int history_index = 0;
    unsigned long long total_cycles = 0;
    unsigned long long last_progress_check = 0;
    MEM_TWO_WORDS last_check_pc = REG.PC;

    // Initialize history
    for (int i = 0; i < HISTORY_SIZE; i++) {
        pc_history[i] = 0xFFFF;
        history_count[i] = 0;
    }

    printf("Running test");
    fflush(stdout);

    // Run the test
    while (total_cycles < MAX_CYCLES) {
        MEM_TWO_WORDS current_pc = REG.PC;

        cpu_step();
        total_cycles++;

        // Check for success
        if (REG.PC == TEST_SUCCESS_ADDRESS) {
            printf("\n\nTest Passed\n");
            printf("Total cycles: %llu\n", total_cycles);
            printf("Final PC: $%04X (success trap)\n", REG.PC);
            printf("A=$%02X X=$%02X Y=$%02X S=$%02X P=$%02X\n",
                   REG.A, REG.X, REG.Y, REG.S, REG.P);
            return 0;
        }

        // Detect infinite loop by checking if PC hasn't changed in 1000000 cycles (increased for test 29)
        if (total_cycles - last_progress_check >= 1000000) {
            if (current_pc == last_check_pc) {
                printf("\n\nTest Failed\n");
                printf("Test trapped at PC=$%04X\n", current_pc);
                printf("Total cycles: %llu\n", total_cycles);
                printf("Registers:\n");
                printf("  A=$%02X X=$%02X Y=$%02X\n", REG.A, REG.X, REG.Y);
                printf("  S=$%02X P=$%02X\n", REG.S, REG.P);
                printf("  Flags: N=%d V=%d B=%d D=%d I=%d Z=%d C=%d\n",
                       (REG.P & FLAG_N) ? 1 : 0,
                       (REG.P & FLAG_V) ? 1 : 0,
                       (REG.P & FLAG_B) ? 1 : 0,
                       (REG.P & FLAG_D) ? 1 : 0,
                       (REG.P & FLAG_I) ? 1 : 0,
                       (REG.P & FLAG_Z) ? 1 : 0,
                       (REG.P & FLAG_C) ? 1 : 0);

                // Read the test_num from $0200 to identify which test failed
                MEM_WORD test_num = bus_read(0x0200);
                printf("\nTest case number: $%02X\n", test_num);
                printf("tests/6502_functional_test/6502_functional_test.lst for details.\n");
                printf("Address $%04X might be related with the issue..\n", current_pc);

                return 1;
            }
            last_progress_check = total_cycles;
            last_check_pc = current_pc;
        }

        // Progress indicator every 1 million cycles
        if (total_cycles % 1000000 == 0) {
            printf(".");
            fflush(stdout);
        }
    }

    // Timeout
    printf("\n\nTest Timeout\n");
    printf("Exceeded maximum cycles (%d)\n", MAX_CYCLES);
    printf("Last PC: $%04X\n", REG.PC);
    printf("Likely a looping issue.\n");

    return 1;
}
