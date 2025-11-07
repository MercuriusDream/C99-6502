/*
 * Simple Debug Test - minimal test to check debugger basics
 */

#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "debugger.h"

int main() {
    printf("=== Simple Debugger Test ===\n\n");

    debugger_init();

    // Configure memory
    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);
    mem_region_add_rom(0x8000, 0x8000);
    mem_region_init();

    // Load simple program
    MEM_WORD test_program[] = {
        0xA9, 0x42,  // LDA #$42
        0xE8,        // INX
        0x00         // BRK
    };

    mem_region_load(0x8000, test_program, sizeof(test_program));
    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, 0x8000);

    cpu_reset();

    printf("Test 1: Disassembly\n");
    debugger_disassemble(0x8000, 3);

    printf("\nTest 2: Add breakpoint\n");
    debugger_add_breakpoint(BP_TYPE_EXEC, 0x8000, "Start");

    printf("\nTest 3: Execute\n");
    for (int i = 0; i < 10; i++) {
        if (REG.PC == 0) break;
        cpu_step();
    }

    printf("\nTest 4: Dump registers\n");
    debugger_dump_registers();

    printf("\nTest 5: Hex dump\n");
    debugger_hexdump(0x8000, 16);

    printf("\nTest Complete!\n");
    debugger_cleanup();

    return 0;
}
