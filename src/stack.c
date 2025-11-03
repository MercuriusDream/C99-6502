#include "stack.h"
#include "types.h"
#include "bus.h"
#include "cpu.h"

void push8(MEM_WORD DATA) {
    bus_write(STACK_START + REG.S--, DATA);
}

void push16(MEM_TWO_WORDS DATA) {
    push8((MEM_WORD)(DATA >> 8));
    push8((MEM_WORD)(DATA & 0xFF));
}

MEM_WORD pop8() {
    return bus_read(STACK_START + ++REG.S);
}

MEM_TWO_WORDS pop16() {
    MEM_WORD low = pop8();
    MEM_WORD high = pop8();
    return (high << 8) | low;
}