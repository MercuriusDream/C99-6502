#include "interrupt.h"
#include "cpu.h"
#include "stack.h"
#include "bus.h"
#include <stdio.h>
#include <string.h>

static INTERRUPT_CONTROLLER ic;
static unsigned long long global_cycle_count = 0;


// Initialization


void interrupt_init(void) {
    memset(&ic, 0, sizeof(ic));
    ic.irq_line = 0;
    ic.irq_line_prev = 0;
    ic.nmi_line = 0;
    ic.nmi_line_prev = 0;
    ic.nmi_pending = 0;
    ic.interrupt_in_progress = 0;
    ic.current_interrupt = INT_NONE;
    ic.interrupt_cycle = 0;
    ic.history_count = 0;
    ic.history_index = 0;
    ic.total_irqs = 0;
    ic.total_nmis = 0;
    ic.total_brks = 0;
}


// Interrupt Line Control


void interrupt_set_irq(int active) {
    ic.irq_line_prev = ic.irq_line;
    ic.irq_line = active;
}

void interrupt_set_nmi(int active) {
    ic.nmi_line_prev = ic.nmi_line;
    ic.nmi_line = active;

    // NMI is edge-triggered (falling edge: 1 -> 0)
    if (ic.nmi_line_prev == 1 && ic.nmi_line == 0) {
        ic.nmi_pending = 1;
    }
}


// Interrupt Polling and Detection


INTERRUPT_TYPE interrupt_poll(void) {
    global_cycle_count++;

    // NMI has highest priority (can hijack IRQ sequence)
    if (ic.nmi_pending) {
        return INT_NMI;
    }

    // IRQ is level-triggered and maskable
    if (ic.irq_line && !GET_FLAG(FLAG_I)) {
        return INT_IRQ;
    }

    return INT_NONE;
}


// Interrupt Sequence


static void record_interrupt(INTERRUPT_TYPE type, MEM_TWO_WORDS pc, MEM_WORD p) {
    INTERRUPT_EVENT* event = &ic.history[ic.history_index];
    event->type = type;
    event->pc = pc;
    event->p = p;
    event->cycle = global_cycle_count;

    ic.history_index = (ic.history_index + 1) % MAX_INTERRUPT_HISTORY;
    if (ic.history_count < MAX_INTERRUPT_HISTORY) {
        ic.history_count++;
    }

    // Update statistics
    switch (type) {
        case INT_IRQ:
            ic.total_irqs++;
            break;
        case INT_NMI:
            ic.total_nmis++;
            break;
        case INT_BRK:
            ic.total_brks++;
            break;
        default:
            break;
    }
}

void interrupt_begin(INTERRUPT_TYPE type) {
    if (ic.interrupt_in_progress) {
        // Check for NMI hijacking
        if (type == INT_NMI && ic.current_interrupt == INT_IRQ) {
            // NMI can hijack IRQ sequence
            printf("[Interrupt] NMI hijacking IRQ sequence\n");
        }
    }

    ic.interrupt_in_progress = 1;
    ic.current_interrupt = type;
    ic.interrupt_cycle = 0;

    // Record the interrupt event
    record_interrupt(type, REG.PC, REG.P);
}

int interrupt_step_cycle(void) {
    if (!ic.interrupt_in_progress) {
        return 1;  // Not in interrupt sequence
    }

    // Real 6502 interrupt sequence takes 7 cycles:
    // Cycle 0-1: Read next instruction bytes (discarded)
    // Cycle 2: Push PCH
    // Cycle 3: Push PCL
    // Cycle 4: Push P
    // Cycle 5: Fetch vector low
    // Cycle 6: Fetch vector high
    //
    // For simplicity, we execute the whole sequence at once
    // but track that it consumes 7 cycles

    if (ic.interrupt_cycle == 0) {
        // Execute the entire interrupt sequence
        MEM_TWO_WORDS vector_addr;
        int set_b_flag = 0;

        switch (ic.current_interrupt) {
            case INT_BRK:
                // BRK: PC was already incremented past BRK opcode
                push16(REG.PC + 1);  // Push return address (skip padding byte)
                set_b_flag = 1;
                vector_addr = IRQ_VECTOR;
                break;

            case INT_IRQ:
                push16(REG.PC);
                set_b_flag = 0;
                vector_addr = IRQ_VECTOR;
                break;

            case INT_NMI:
                push16(REG.PC);
                set_b_flag = 0;
                vector_addr = NMI_VECTOR;
                ic.nmi_pending = 0;  // Clear pending NMI
                break;

            default:
                ic.interrupt_in_progress = 0;
                return 1;
        }

        // Push processor status
        if (set_b_flag) {
            push8(REG.P | FLAG_B | FLAG_U);
        } else {
            push8(REG.P | FLAG_U);
        }

        // Set interrupt disable flag
        SET_FLAG(FLAG_I);

        // Wake CPU from WAI
        cpu_set_waiting(0);

        // Jump to interrupt vector
        REG.PC = bus_read16(vector_addr);

        // Move to next cycle
        ic.interrupt_cycle++;
        return 0;  // Still in progress
    }

    // Cycles 1-6: Wait for interrupt sequence to complete
    ic.interrupt_cycle++;
    if (ic.interrupt_cycle >= 7) {
        ic.interrupt_in_progress = 0;
        ic.current_interrupt = INT_NONE;
        ic.interrupt_cycle = 0;
        return 1;  // Complete
    }

    return 0;  // Still in progress
}

int interrupt_is_active(void) {
    return ic.interrupt_in_progress;
}

INTERRUPT_TYPE interrupt_get_current(void) {
    return ic.current_interrupt;
}


// Debugging and History


void interrupt_dump_history(void) {
    printf("\n[Interrupt Controller] Interrupt History (last %d):\n", ic.history_count);

    if (ic.history_count == 0) {
        printf("  No interrupts recorded\n");
        return;
    }

    // Print in chronological order
    int start_index;
    if (ic.history_count < MAX_INTERRUPT_HISTORY) {
        start_index = 0;
    } else {
        start_index = ic.history_index;
    }

    for (int i = 0; i < ic.history_count; i++) {
        int idx = (start_index + i) % MAX_INTERRUPT_HISTORY;
        INTERRUPT_EVENT* event = &ic.history[idx];

        const char* type_str = "UNKNOWN";
        switch (event->type) {
            case INT_IRQ:   type_str = "IRQ"; break;
            case INT_NMI:   type_str = "NMI"; break;
            case INT_BRK:   type_str = "BRK"; break;
            case INT_RESET: type_str = "RESET"; break;
            default: break;
        }

        printf("  [%3d] Cycle %llu: %s at PC=$%04X P=$%02X\n",
               i + 1, event->cycle, type_str, event->pc, event->p);
    }
}

void interrupt_dump_stats(void) {
    printf("\n[Interrupt Controller] Statistics:\n");
    printf("  Total IRQs:  %llu\n", ic.total_irqs);
    printf("  Total NMIs:  %llu\n", ic.total_nmis);
    printf("  Total BRKs:  %llu\n", ic.total_brks);
    printf("  Total:       %llu\n", ic.total_irqs + ic.total_nmis + ic.total_brks);
}

void interrupt_clear_history(void) {
    ic.history_count = 0;
    ic.history_index = 0;
    ic.total_irqs = 0;
    ic.total_nmis = 0;
    ic.total_brks = 0;
    printf("[Interrupt Controller] History and statistics cleared\n");
}

INTERRUPT_CONTROLLER* interrupt_get_controller(void) {
    return &ic;
}
