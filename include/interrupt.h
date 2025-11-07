#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

// Interrupt vectors
#define IRQ_VECTOR 0xFFFE
#define NMI_VECTOR 0xFFFA
#define RESET_VECTOR 0xFFFC

// Interrupt types
typedef enum {
    INT_NONE = 0,
    INT_IRQ,
    INT_NMI,
    INT_BRK,
    INT_RESET
} INTERRUPT_TYPE;

// Interrupt history entry for debugging
typedef struct {
    INTERRUPT_TYPE type;
    MEM_TWO_WORDS pc;
    MEM_WORD p;
    unsigned long long cycle;
} INTERRUPT_EVENT;

#define MAX_INTERRUPT_HISTORY 32

// Interrupt controller state
typedef struct {
    // IRQ line state (level-triggered input)
    int irq_line;
    int irq_line_prev;

    // NMI edge detection
    int nmi_line;
    int nmi_line_prev;
    int nmi_pending;  // NMI triggered by falling edge

    // Interrupt sequence tracking
    int interrupt_in_progress;
    INTERRUPT_TYPE current_interrupt;
    int interrupt_cycle;  // Current cycle in interrupt sequence (0-6)

    // Interrupt history for debugging
    INTERRUPT_EVENT history[MAX_INTERRUPT_HISTORY];
    int history_count;
    int history_index;

    // Statistics
    unsigned long long total_irqs;
    unsigned long long total_nmis;
    unsigned long long total_brks;
} INTERRUPT_CONTROLLER;

// Initialize interrupt controller
void interrupt_init(void);

// Set interrupt lines (called by peripherals/test code)
void interrupt_set_irq(int active);  // 1 = active (low on real hardware)
void interrupt_set_nmi(int active);  // 1 = active (low on real hardware)

// Poll for pending interrupts (called by CPU each cycle)
INTERRUPT_TYPE interrupt_poll(void);

// Begin interrupt sequence
void interrupt_begin(INTERRUPT_TYPE type);

// Execute one cycle of interrupt sequence
// Returns 1 if interrupt sequence is complete, 0 if still in progress
int interrupt_step_cycle(void);

// Check if interrupt is in progress
int interrupt_is_active(void);

// Get current interrupt type
INTERRUPT_TYPE interrupt_get_current(void);

// Interrupt history and debugging
void interrupt_dump_history(void);
void interrupt_dump_stats(void);
void interrupt_clear_history(void);

// Get interrupt controller state (for debugger)
INTERRUPT_CONTROLLER* interrupt_get_controller(void);

#endif
