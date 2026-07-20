#ifndef INSTRUCTIONS_HANDLERS_H
#define INSTRUCTIONS_HANDLERS_H

#include "types.h"

typedef void (*instr_fn)(void);

// Illegal opcode handler
void op_illegal(void);

// Variant-aware instruction dispatch
void instr_dispatch(void);

extern const instr_fn INSTR_HANDLERS[256];

#endif