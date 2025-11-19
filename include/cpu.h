#ifndef CPU_H
#define CPU_H

#include "types.h"

extern MEM_WORD OPCODE;
extern SIGNED_MEM_WORD REL_OFFSET;
extern MEM_TWO_WORDS CYCLES;

// CPU variant configuration
void cpu_set_variant(CPU_VARIANT variant);
CPU_VARIANT cpu_get_variant();

// CPU state management (for 65C02 WAI and STP)
void cpu_set_waiting(int waiting);
void cpu_set_stopped(int stopped);
int cpu_is_waiting(void);
int cpu_is_stopped(void);

void cpu_init(void);
void cpu_reset();
void cpu_step();
void cpu_run(MEM_TWO_WORDS MAX_CYCLES);
void cpu_irq();
void cpu_nmi();

MEM_WORD fetch8();
MEM_TWO_WORDS fetch16();

void set_zn(MEM_WORD V);

#endif
