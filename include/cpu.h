#ifndef CPU_H
#define CPU_H

#include "types.h"

// CPU variant configuration
void cpu_set_variant(CPU_VARIANT variant);
CPU_VARIANT cpu_get_variant();

void cpu_reset();
void cpu_step();
void cpu_run(MEM_TWO_WORDS MAX_CYCLES);
void cpu_irq();
void cpu_nmi();

MEM_WORD fetch8();
MEM_TWO_WORDS fetch16();

void set_zn(MEM_WORD V);

#endif
