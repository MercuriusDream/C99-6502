/*
 * Interrupt Controller Test Program
 * Demonstrates interrupt handling: BRK, IRQ, NMI, edge detection
 */

#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "interrupt.h"

int main() {
    printf("=== MOS 6502 Interrupt Controller Test ===\n\n");

    // Initialize
    cpu_init();  // This initializes the interrupt controller

    // Configure memory
    mem_region_clear();
    if (mem_region_add_ram(0x0000, 0x8000) != 0) {
        printf("Error: Failed to allocate RAM\n");
        return 1;
    }
    if (mem_region_add_rom(0x8000, 0x8000) != 0) {
        printf("Error: Failed to allocate ROM\n");
        return 1;
    }
    mem_region_init();

    // ========================================================================
    // Test 1: BRK Instruction (Software Interrupt)
    // ========================================================================
    printf("--- Test 1: BRK Instruction ---\n");

    // Program that executes BRK
    MEM_WORD brk_program[] = {
        0xA9, 0x11,  // LDA #$11
        0x00,        // BRK
        0x00,        // Padding byte (skipped by BRK)
        0xA9, 0x22,  // LDA #$22 (should not execute)
    };

    // BRK handler at $9000
    MEM_WORD brk_handler[] = {
        0xA9, 0x99,  // LDA #$99  (mark that we entered interrupt)
        0x8D, 0x00, 0x02,  // STA $0200
        0x40         // RTI
    };

    mem_region_load(0x8000, brk_program, sizeof(brk_program));
    mem_region_load(0x9000, brk_handler, sizeof(brk_handler));

    // Set interrupt vectors
    mem_region_set_vector(0xFFFC, 0x8000);  // Reset vector
    mem_region_set_vector(0xFFFE, 0x9000);  // IRQ/BRK vector

    cpu_reset();
    printf("Initial state: PC=$%04X A=$%02X\n", REG.PC, REG.A);

    // Execute until BRK
    for (int i = 0; i < 10; i++) {
        cpu_step();
        if (interrupt_is_active()) {
            printf("  Interrupt active: %d\n", interrupt_get_current());
        }
        if (REG.PC == 0) break;  // Stop after RTI returns to 0
    }

    printf("After BRK: PC=$%04X A=$%02X, Mem[$0200]=$%02X\n",
           REG.PC, REG.A, bus_read(0x0200));
    printf("Expected: A=$99 (from interrupt handler)\n");

    // ========================================================================
    // Test 2: IRQ (Maskable Interrupt)
    // ========================================================================
    printf("\n--- Test 2: IRQ (Maskable Interrupt) ---\n");

    // Simple program that loops
    MEM_WORD irq_program[] = {
        0xA9, 0x10,  // $8000: LDA #$10
        0x8D, 0x10, 0x02,  // STA $0210
        0x58,        // CLI (enable interrupts)
        0xE8,        // $8006: INX  (loop point)
        0x4C, 0x06, 0x80,  // JMP $8006
    };

    // IRQ handler
    MEM_WORD irq_handler[] = {
        0xA9, 0xAA,  // LDA #$AA
        0x8D, 0x11, 0x02,  // STA $0211
        0x40         // RTI
    };

    mem_region_load(0x8000, irq_program, sizeof(irq_program));
    mem_region_load(0x9100, irq_handler, sizeof(irq_handler));
    mem_region_set_vector(0xFFFC, 0x8000);
    mem_region_set_vector(0xFFFE, 0x9100);  // IRQ vector

    cpu_reset();
    printf("Starting program at $%04X\n", REG.PC);

    // Run a few instructions
    for (int i = 0; i < 5; i++) {
        cpu_step();
    }

    printf("Before IRQ: X=$%02X, I flag=%d\n", REG.X, GET_FLAG(FLAG_I));

    // Trigger IRQ
    printf("Triggering IRQ...\n");
    cpu_irq();

    // Continue execution
    for (int i = 0; i < 5; i++) {
        cpu_step();
    }

    printf("After IRQ: Mem[$0211]=$%02X (expected $AA)\n", bus_read(0x0211));

    // ========================================================================
    // Test 3: NMI (Non-Maskable Interrupt, Edge-Triggered)
    // ========================================================================
    printf("\n--- Test 3: NMI (Non-Maskable Interrupt) ---\n");

    // NMI handler
    MEM_WORD nmi_handler[] = {
        0xA9, 0xBB,  // LDA #$BB
        0x8D, 0x12, 0x02,  // STA $0212
        0x40         // RTI
    };

    mem_region_load(0x9200, nmi_handler, sizeof(nmi_handler));
    mem_region_set_vector(0xFFFA, 0x9200);  // NMI vector

    printf("Triggering NMI...\n");
    cpu_nmi();

    // Continue execution
    for (int i = 0; i < 5; i++) {
        cpu_step();
    }

    printf("After NMI: Mem[$0212]=$%02X (expected $BB)\n", bus_read(0x0212));

    // ========================================================================
    // Test 4: Multiple Interrupts (NMI Priority)
    // ========================================================================
    printf("\n--- Test 4: Multiple Interrupts ---\n");

    // Test that NMI has priority over IRQ
    interrupt_set_irq(1);  // Assert IRQ
    interrupt_set_nmi(1);  // Assert NMI line
    interrupt_set_nmi(0);  // Create falling edge for NMI

    INTERRUPT_TYPE pending = interrupt_poll();
    printf("With both IRQ and NMI pending: %s has priority\n",
           pending == INT_NMI ? "NMI" : "IRQ");

    // ========================================================================
    // Test 5: Interrupt History and Statistics
    // ========================================================================
    printf("\n--- Test 5: Interrupt History & Statistics ---\n");

    interrupt_dump_history();
    interrupt_dump_stats();

    printf("\n=== Interrupt Controller Test Complete ===\n");
    return 0;
}
