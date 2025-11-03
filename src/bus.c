#include "bus.h"
#include "types.h"

MEM_WORD bus_read(MEM_TWO_WORDS ADDR) {
    return BUS.READ ? BUS.READ(ADDR, BUS.CTX) : 0;
}

void bus_write(MEM_TWO_WORDS ADDR, MEM_WORD DATA) {
    if (BUS.WRITE) BUS.WRITE(ADDR, DATA, BUS.CTX);
}

MEM_TWO_WORDS bus_read16(MEM_TWO_WORDS addr) {
    MEM_WORD lo = bus_read(addr);
    MEM_WORD hi = bus_read(addr + 1);
    return lo | ((MEM_TWO_WORDS)hi << 8);
}
