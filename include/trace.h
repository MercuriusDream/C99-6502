#ifndef TRACE_H
#define TRACE_H

#include "types.h"

void trace_before(MEM_TWO_WORDS PC, MEM_WORD OPCODE);
void trace_after(void);
void trace_set_enabled(int ENABLED);

#endif
