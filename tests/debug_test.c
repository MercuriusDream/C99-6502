/*
 * Debug Test Program
 * Demonstrates the new debugging and profiling features
 */

#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "debugger.h"
#include "trace.h"

int main(int argc, char** argv) {
    printf("=== MOS 6502 Debugger Test ===\n\n");

    // Initialize debugger
    debugger_init();

    // Configure memory (32KB RAM + 32KB ROM)
    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);
    mem_region_add_rom(0x8000, 0x8000);
    mem_region_init();

    // Load a test program
    // This program: loops, modifies memory, does arithmetic
    MEM_WORD test_program[] = {
        // $8000: Initialize
        0xA2, 0x00,        // LDX #$00
        0xA0, 0x00,        // LDY #$00

        // $8004: Loop start
        0xA9, 0x42,        // LDA #$42
        0x8D, 0x00, 0x02,  // STA $0200
        0xE8,              // INX
        0xC8,              // INY

        // $800C: Add
        0x18,              // CLC
        0x69, 0x01,        // ADC #$01
        0x8D, 0x01, 0x02,  // STA $0201

        // $8012: Check if done
        0xE0, 0x05,        // CPX #$05
        0xD0, 0xED,        // BNE $8004 (loop back)

        // $8016: Done
        0x00               // BRK
    };

    mem_region_load(0x8000, test_program, sizeof(test_program));
    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, 0x8000);

    printf("Test program loaded at $8000\n");
    printf("Program description:\n");
    printf("  - Initialize X=0, Y=0\n");
    printf("  - Loop 5 times:\n");
    printf("    * Store $42 to $0200\n");
    printf("    * Increment X and Y\n");
    printf("    * Add 1 to A and store to $0201\n");
    printf("  - Break when X reaches 5\n\n");

    // Reset CPU
    cpu_reset();

    // ========================================================================
    // Test 1: Basic Disassembly
    // ========================================================================
    printf("\n--- Test 1: Disassembly ---\n");
    debugger_disassemble(0x8000, 15);

    // ========================================================================
    // Test 2: Breakpoints
    // ========================================================================
    printf("\n--- Test 2: Breakpoints ---\n");
    debugger_add_breakpoint(BP_TYPE_EXEC, 0x8004, "Loop start");
    debugger_add_breakpoint(BP_TYPE_EXEC, 0x8016, "Loop end");
    debugger_list_breakpoints();

    // ========================================================================
    // Test 3: Watchpoints
    // ========================================================================
    printf("\n--- Test 3: Watchpoints ---\n");
    debugger_add_watchpoint(0x0200, "Loop counter");
    debugger_add_watchpoint(0x0201, "Accumulator storage");
    debugger_list_watchpoints();

    // ========================================================================
    // Test 4: Execution with profiling
    // ========================================================================
    printf("\n--- Test 4: Execution with Profiling ---\n");
    profiler_init();

    printf("\nExecuting program...\n");
    for (int i = 0; i < 200; i++) {
        MEM_TWO_WORDS pc_before = REG.PC;
        MEM_WORD opcode = bus_read(REG.PC);

        // Check for execution breakpoint
        if (debugger_check_breakpoint(BP_TYPE_EXEC, REG.PC)) {
            printf("\n");
            debugger_dump_registers();
        }

        cpu_step();

        // Record profiling data (simplified - normally done in cpu_step)
        profiler_record_instruction(pc_before, opcode, 2);  // Using 2 as dummy cycle count

        // Check watchpoints
        debugger_check_watchpoints();

        // Break on BRK
        if (REG.PC == 0) {
            printf("\nBRK encountered at cycle %d\n", i);
            break;
        }
    }

    // ========================================================================
    // Test 5: Final State Inspection
    // ========================================================================
    printf("\n--- Test 5: Final State Inspection ---\n");
    debugger_dump_registers();

    printf("\n--- Test 6: Memory Inspection ---\n");
    debugger_hexdump(0x0200, 16);

    // ========================================================================
    // Test 7: Profiling Statistics
    // ========================================================================
    printf("\n--- Test 7: Profiling Statistics ---\n");
    profiler_dump_stats();
    profiler_dump_hotspots(10);

    // ========================================================================
    // Test 8: Memory Search
    // ========================================================================
    printf("\n--- Test 8: Memory Search ---\n");
    MEM_WORD pattern[] = { 0xA9, 0x42 };  // LDA #$42
    debugger_search_memory(0x8000, 0x8020, pattern, 2);

    // ========================================================================
    // Test 9: Stack Inspection
    // ========================================================================
    printf("\n--- Test 9: Stack Inspection ---\n");
    debugger_dump_stack();

    // Cleanup
    debugger_cleanup();

    printf("\n=== Debugger Test Complete ===\n");
    return 0;
}
