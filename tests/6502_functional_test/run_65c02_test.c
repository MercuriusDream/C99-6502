#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "instructions_table.h"
#include "instructions_implementation.h"
#include "memory.h"
#include "loader.h"
#include "trace.h"

#define TEST_START_ADDRESS 0x0400
#define TEST_SUCCESS_ADDRESS 0x24F1  // 65C02 extended opcodes test success address
#define MAX_CYCLES 100000000  // 100 million cycles should be enough

int main(int argc, char** argv) {
    const char* test_file = "tests/6502_functional_test/65C02_extended_opcodes_test.bin";
    int enable_trace = 0;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0) {
            enable_trace = 1;
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            test_file = argv[++i];
        }
    }

    // Set CPU variant to 65C02
    cpu_set_variant(CPU_VARIANT_CMOS_65C02);

    printf("Klaus Dormann 65C02 Extended Opcodes Test\n");
    printf("CPU Variant: CMOS 65C02\n");
    printf("Test file: %s\n\n", test_file);

    // Setup memory - entire 64KB as RAM for the test
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

    unsigned long long total_cycles = 0;
    unsigned long long last_progress_check = 0;
    MEM_TWO_WORDS last_check_pc = REG.PC;

    printf("Running test");
    fflush(stdout);

    // Run the test
    while (total_cycles < MAX_CYCLES) {
        MEM_TWO_WORDS current_pc = REG.PC;

        cpu_step();
        total_cycles++;

        // Check for success
        if (REG.PC == TEST_SUCCESS_ADDRESS) {
            printf("\n\n✓ Test Passed!\n");
            printf("Total cycles: %llu (%.2f million)\n", total_cycles, total_cycles / 1000000.0);
            printf("Final PC: $%04X (success trap)\n", REG.PC);
            printf("A=$%02X X=$%02X Y=$%02X S=$%02X P=$%02X\n",
                   REG.A, REG.X, REG.Y, REG.S, REG.P);
            printf("\nAll 65C02 extended opcodes working correctly!\n");
            return 0;
        }

        // Detect infinite loop by checking if PC hasn't changed in 1000000 cycles
        if (total_cycles - last_progress_check >= 1000000) {
            if (current_pc == last_check_pc) {
                printf("\n\n✗ Test Failed\n");
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
                printf("Check tests/6502_functional_test/65C02_extended_opcodes_test.lst for details.\n");
                printf("Address $%04X might be related with the issue.\n", current_pc);

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
    printf("\n\n✗ Test Timeout\n");
    printf("Exceeded maximum cycles (%d)\n", MAX_CYCLES);
    printf("Last PC: $%04X\n", REG.PC);
    printf("Likely a looping issue.\n");

    return 1;
}
