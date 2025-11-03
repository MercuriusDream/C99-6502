#ifndef BUS_H
#define BUS_H
#include "types.h"

void bus_write(MEM_TWO_WORDS ADDR, MEM_WORD DATA);
MEM_WORD bus_read(MEM_TWO_WORDS ADDR);
MEM_TWO_WORDS bus_read16(MEM_TWO_WORDS addr);

#endif
