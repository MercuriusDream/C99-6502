/*
 * Simple Debug Test - minimal test to check debugger basics.
 * Real assertion-based.
 */

#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "debugger.h"
#include "interrupt.h"

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

int main() {
    printf("=== Simple Debugger Test ===\n\n");

    debugger_init();

    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);
    mem_region_add_rom(0x8000, 0x8000);
    mem_region_init();

    MEM_WORD test_program[] = {
        0xA9, 0x42,  // LDA #$42
        0xE8,        // INX
        0x00         // BRK
    };

    mem_region_load(0x8000, test_program, sizeof(test_program));
    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, 0x8000);
    mem_region_set_vector(IRQ_VECTOR, 0x9000);
    cpu_reset();

    CHECK16(REG.PC, 0x8000, "reset PC=$8000");

    printf("\nTest 1: Disassembly\n");
    debugger_disassemble(0x8000, 3);
    printf("PASS: disassemble completed without crash\n");

    printf("\nTest 2: Add breakpoint\n");
    int bp = debugger_add_breakpoint(BP_TYPE_EXEC, 0x8000, "Start");
    CHECK_INT(bp, 0, "add breakpoint returns index 0");

    /* The breakpoint at $8000 should report a hit before we execute it. */
    CHECK_INT(debugger_check_breakpoint(BP_TYPE_EXEC, 0x8000), 1,
              "check_breakpoint hits at $8000");
    CHECK_INT(debugger_check_breakpoint(BP_TYPE_EXEC, 0x8001), 0,
              "check_breakpoint misses at $8001");

    printf("\nTest 3: Execute\n");
    int steps = 0;
    for (int i = 0; i < 10; i++) {
        cpu_step();
        steps++;
        /* Stop once execution leaves the tiny program region ($8000-$8003).
           After LDA->PC=$8002, INX->PC=$8003, BRK->PC=$9000 (handler). */
        if (REG.PC < 0x8000 || REG.PC > 0x8003) break;
    }
    /* LDA #$42 (1) + INX (1) + BRK (1) = 3 steps; BRK sets PC=$9000 (vector). */
    CHECK_INT(steps, 3, "executed 3 instructions (LDA, INX, BRK)");

    printf("\nTest 4: Dump registers\n");
    debugger_dump_registers();
    CHECK8(REG.A, 0x42, "A == $42 after LDA #$42");
    CHECK8(REG.X, 0x01, "X == $01 after INX");
    /* BRK services the interrupt: PC should now be the IRQ/BRK handler. */
    CHECK16(REG.PC, 0x9000, "PC == $9000 (BRK/IRQ vector) after BRK");

    printf("\nTest 5: Hex dump\n");
    debugger_hexdump(0x8000, 16);
    /* Verify the program bytes are readable as expected. */
    CHECK8(bus_read(0x8000), 0xA9, "mem[0x8000] == $A9 (LDA opcode)");
    CHECK8(bus_read(0x8001), 0x42, "mem[0x8001] == $42 (LDA operand)");
    CHECK8(bus_read(0x8002), 0xE8, "mem[0x8002] == $E8 (INX)");
    CHECK8(bus_read(0x8003), 0x00, "mem[0x8003] == $00 (BRK)");

    debugger_cleanup();

    printf("\n=== Simple Debugger Test Complete ===\n");
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}