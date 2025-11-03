#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

// Region-based memory interface
void mem_region_clear();
int mem_region_add_ram(MEM_TWO_WORDS START, MEM_TWO_WORDS SIZE);
int mem_region_add_rom(MEM_TWO_WORDS START, MEM_TWO_WORDS SIZE);
int mem_region_add_io(MEM_TWO_WORDS START, MEM_TWO_WORDS SIZE,
                      bus_read_fn READ_HANDLER, bus_write_fn WRITE_HANDLER, void* CTX);
void mem_region_init();

// Helper functions
int mem_region_load(MEM_TWO_WORDS ADDR, const MEM_WORD* DATA, MEM_TWO_WORDS LEN);
int mem_region_set_vector(MEM_TWO_WORDS VEC, MEM_TWO_WORDS DEST);
MEM_WORD* mem_region_get_ptr(MEM_TWO_WORDS ADDR);

#endif