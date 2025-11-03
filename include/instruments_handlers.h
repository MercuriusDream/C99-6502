#ifndef INSTRUMENTS_HANDLERS_H
#define INSTRUMENTS_HANDLERS_H

#include "types.h"

typedef void (*instr_fn)(void);
static void op_illegal();

extern const instr_fn INSTR_HANDLERS[256];

#endif