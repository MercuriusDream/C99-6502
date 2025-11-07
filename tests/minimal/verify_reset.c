#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "loader.h"

extern T_REGISTER REG;

int main() {
    printf("C99-6502 Reset Verification\n");

    // Initialize memory regions
    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);  // 32KB RAM
    mem_region_add_rom(0x8000, 0x8000);  // 32KB ROM

    // Load ROM and setup
    load_bin_region("tests/minimal/test.bin", 0x8000);
    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, 0x8000);
    mem_region_init();

    // Set registers to non-zero values
    REG.A = 0xFF;
    REG.X = 0xFF;
    REG.Y = 0xFF;

    cpu_reset();

    // Verify results
    int passed = 0;
    int failed = 0;

    printf("Test Results:\n\n");

    printf("Test 1 - A Register Reset: $%02X (expected $00) ... %s\n",
           REG.A, (REG.A == 0x00) ? "PASS" : "FAIL");
    (REG.A == 0x00) ? passed++ : failed++;

    printf("Test 2 - X Register Reset: $%02X (expected $00) ... %s\n",
           REG.X, (REG.X == 0x00) ? "PASS" : "FAIL");
    (REG.X == 0x00) ? passed++ : failed++;

    printf("Test 3 - Y Register Reset: $%02X (expected $00) ... %s\n",
           REG.Y, (REG.Y == 0x00) ? "PASS" : "FAIL");
    (REG.Y == 0x00) ? passed++ : failed++;

    printf("Total: %d passed, %d failed\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}
