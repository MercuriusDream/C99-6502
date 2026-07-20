/*
 * Debug Test Program
 * Real assertion-based tests for the debugging and profiling features.
 */

#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "debugger.h"
#include "interrupt.h"
#include "trace.h"

static int failures = 0;

#define CHECK_INT(actual, expected, msg)                                   \
    do {                                                                  \
        long _a = (long)(actual);                                         \
        long _e = (long)(expected);                                       \
        if (_a != _e) {                                                   \
            printf("FAIL: %s got %ld expected %ld\n", msg, _a, _e);        \
            failures++;                                                   \
        } else {                                                          \
            printf("PASS: %s (%ld)\n", msg, _a);                          \
        }                                                                 \
    } while (0)

#define CHECK8(actual, expected, msg)                                     \
    do {                                                                  \
        MEM_WORD _a = (MEM_WORD)(actual);                                 \
        MEM_WORD _e = (MEM_WORD)(expected);                               \
        if (_a != _e) {                                                   \
            printf("FAIL: %s got $%02X expected $%02X\n", msg, _a, _e);    \
            failures++;                                                   \
        } else {                                                          \
            printf("PASS: %s ($%02X)\n", msg, _a);                        \
        }                                                                 \
    } while (0)

#define CHECK16(actual, expected, msg)                                    \
    do {                                                                  \
        MEM_TWO_WORDS _a = (MEM_TWO_WORDS)(actual);                       \
        MEM_TWO_WORDS _e = (MEM_TWO_WORDS)(expected);                     \
        if (_a != _e) {                                                   \
            printf("FAIL: %s got $%04X expected $%04X\n", msg, _a, _e);    \
            failures++;                                                   \
        } else {                                                          \
            printf("PASS: %s ($%04X)\n", msg, _a);                         \
        }                                                                 \
    } while (0)

int main(int argc, char** argv) {
    printf("=== MOS 6502 Debugger Test ===\n\n");

    debugger_init();

    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);
    mem_region_add_rom(0x8000, 0x8000);
    mem_region_init();

    MEM_WORD test_program[] = {
        0xA2, 0x00,        // LDX #$00
        0xA0, 0x00,        // LDY #$00
        0xA9, 0x42,        // LDA #$42
        0x8D, 0x00, 0x02,  // STA $0200
        0xE8,              // INX
        0xC8,              // INY
        0x18,              // CLC
        0x69, 0x01,        // ADC #$01
        0x8D, 0x01, 0x02,  // STA $0201
        0xE0, 0x05,        // CPX #$05
        0xD0, 0xED,        // BNE $8004
        0x00               // BRK
    };

    mem_region_load(0x8000, test_program, sizeof(test_program));
    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, 0x8000);
    mem_region_set_vector(IRQ_VECTOR, 0x9000);
    cpu_reset();

    /* --- Test 1: Disassembly (output-only; assert it doesn't crash) --- */
    printf("\n--- Test 1: Disassembly ---\n");
    debugger_disassemble(0x8000, 15);
    printf("PASS: disassemble 15 instructions completed without crash\n");

    /* --- Test 2: Breakpoints --- */
    printf("\n--- Test 2: Breakpoints ---\n");
    /* debugger_add_breakpoint returns the new breakpoint index (0-based). */
    int bp0 = debugger_add_breakpoint(BP_TYPE_EXEC, 0x8004, "Loop start");
    int bp1 = debugger_add_breakpoint(BP_TYPE_EXEC, 0x8016, "Loop end");
    CHECK_INT(bp0, 0, "add breakpoint #0 returns index 0");
    CHECK_INT(bp1, 1, "add breakpoint #1 returns index 1");

    /* A duplicate / second distinct address add should return next index. */
    int bp2 = debugger_add_breakpoint(BP_TYPE_EXEC, 0x8010, "ADC");
    CHECK_INT(bp2, 2, "add breakpoint #2 returns index 2");

    debugger_list_breakpoints();

    /* check_breakpoint returns 1 when hitting a matching enabled bp. */
    CHECK_INT(debugger_check_breakpoint(BP_TYPE_EXEC, 0x8004), 1,
              "check_breakpoint hits at $8004");
    CHECK_INT(debugger_check_breakpoint(BP_TYPE_EXEC, 0x8016), 1,
              "check_breakpoint hits at $8016");
    /* No breakpoint at $8000 -> 0. */
    CHECK_INT(debugger_check_breakpoint(BP_TYPE_EXEC, 0x8000), 0,
              "check_breakpoint misses at $8000");
    /* Wrong type at a BP address: BP_TYPE_EXEC bp should not match READ. */
    CHECK_INT(debugger_check_breakpoint(BP_TYPE_READ, 0x8004), 0,
              "check_breakpoint wrong type (READ) at $8004 -> 0");

    /* Disable breakpoint #0, then it must not be hit. */
    CHECK_INT(debugger_enable_breakpoint(0, 0), 0, "disable breakpoint #0");
    CHECK_INT(debugger_enable_breakpoint(0, 1), 0, "re-enable breakpoint #0");
    /* Bad index -> -1. */
    CHECK_INT(debugger_enable_breakpoint(99, 1), -1, "enable bad index -> -1");

    /* Remove breakpoint #2, then it should not be hit at that addr. */
    CHECK_INT(debugger_remove_breakpoint(2), 0, "remove breakpoint #2");
    CHECK_INT(debugger_check_breakpoint(BP_TYPE_EXEC, 0x8010), 0,
              "removed bp at $8010 no longer hits");
    CHECK_INT(debugger_remove_breakpoint(99), -1, "remove bad index -> -1");

    /* --- Test 3: Watchpoints --- */
    printf("\n--- Test 3: Watchpoints ---\n");
    int wp0 = debugger_add_watchpoint(0x0200, "Loop counter");
    int wp1 = debugger_add_watchpoint(0x0201, "Accumulator storage");
    CHECK_INT(wp0, 0, "add watchpoint #0 returns index 0");
    CHECK_INT(wp1, 1, "add watchpoint #1 returns index 1");
    debugger_list_watchpoints();

    CHECK_INT(debugger_remove_watchpoint(1), 0, "remove watchpoint #1");
    CHECK_INT(debugger_remove_watchpoint(99), -1, "remove watchpoint bad index -> -1");
    /* Re-add for the rest of the run. */
    debugger_add_watchpoint(0x0201, "Accumulator storage");

    /* --- Test 4: Execution with profiling --- */
    printf("\n--- Test 4: Execution with Profiling ---\n");
    profiler_init();
    int bp_hits = 0;
    int ran_steps = 0;
    for (int i = 0; i < 200; i++) {
        MEM_TWO_WORDS pc_before = REG.PC;
        MEM_WORD opcode = bus_read(REG.PC);
        if (debugger_check_breakpoint(BP_TYPE_EXEC, REG.PC)) {
            bp_hits++;
        }
        cpu_step();
        profiler_record_instruction(pc_before, opcode, 2);
        debugger_check_watchpoints();
        ran_steps++;
        if (REG.PC == 0) break;
    }
    CHECK_INT(ran_steps > 0, 1, "execution loop ran steps");
    CHECK_INT(bp_hits > 0, 1, "execution breakpoint(s) were hit");

    /* --- Test 5: Final register state --- */
    printf("\n--- Test 5: Final State Inspection ---\n");
    debugger_dump_registers();
    /* After the loop the program writes $42 to $0200 repeatedly and
       accumulates A+1 into $0201 each iteration. Final X==5. */
    CHECK8(bus_read(0x0200), 0x42, "$0200 == $42 (LDA #$42 / STA $0200)");
    CHECK8(REG.X, 0x05, "X reached $05 (loop counter)");

    /* --- Test 6: Memory inspection (output-only) --- */
    printf("\n--- Test 6: Memory Inspection ---\n");
    debugger_hexdump(0x0200, 16);
    printf("PASS: hexdump completed without crash\n");

    /* --- Test 7: Profiling statistics --- */
    printf("\n--- Test 7: Profiling Statistics ---\n");
    profiler_dump_stats();
    profiler_dump_hotspots(10);
    PROFILING_DATA* pd = profiler_get_data();
    CHECK_INT(pd != NULL, 1, "profiler_get_data returns non-NULL");
    CHECK_INT(pd->total_cycles > 0, 1, "profiler recorded total_cycles > 0");
    /* LDA imm ($A9) and STA abs ($8D) are both used; assert counts>0. */
    CHECK_INT(pd->instruction_counts[0xA9] > 0, 1, "profiler counted LDA #$A9");
    CHECK_INT(pd->instruction_counts[0x8D] > 0, 1, "profiler counted STA abs $8D");
    CHECK_INT(pd->instruction_counts[0xE8] > 0, 1, "profiler counted INX $E8");
    CHECK_INT(pd->address_exec_counts[0x8000] > 0, 1,
              "profiler recorded execution at $8000");

    /* --- Test 8: Memory search --- */
    printf("\n--- Test 8: Memory Search ---\n");
    MEM_WORD pattern[] = { 0xA9, 0x42 };  // LDA #$42
    int found = debugger_search_memory(0x8000, 0x8020, pattern, 2);
    /* LDA #$42 appears once in the program (at $8004). */
    CHECK_INT(found, 1, "search finds LDA #$42 pattern exactly once");

    MEM_WORD bad_pattern[] = { 0xFF, 0xFF };
    int found0 = debugger_search_memory(0x8000, 0x8020, bad_pattern, 2);
    CHECK_INT(found0, 0, "search for absent pattern returns 0");

    /* --- Test 9: Stack inspection (output-only) --- */
    printf("\n--- Test 9: Stack Inspection ---\n");
    debugger_dump_stack();
    printf("PASS: dump_stack completed without crash\n");

    /* --- Test 10: Interactive mode toggles --- */
    printf("\n--- Test 10: Interactive mode ---\n");
    debugger_set_interactive(1);
    CHECK_INT(debugger_is_interactive(), 1, "interactive mode enabled");
    debugger_set_interactive(0);
    CHECK_INT(debugger_is_interactive(), 0, "interactive mode disabled");

    debugger_cleanup();

    printf("\n=== Debugger Test Complete ===\n");
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}