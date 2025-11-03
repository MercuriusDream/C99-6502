#include "trace.h"
#include "cpu.h"
#include "instruments_table.h"
#include <stdio.h>

static int g_enabled = 0;

void trace_set_enabled(int ENABLED) {
    g_enabled = ENABLED;
}

void trace_before(MEM_TWO_WORDS PC, MEM_WORD OPCODE) {
    if (!g_enabled) return;

    const INST* META = &INST_TABLE[OPCODE>>4][OPCODE&0x0F];

    printf("%04X  %02X  %-4s  A:%02X X:%02X Y:%02X P:%02X SP:%02X",
           PC, OPCODE, META->CMD, REG.A, REG.X, REG.Y, REG.P, REG.S);
}

void trace_after(void) {
    if (!g_enabled) return;
    printf("\n");
}
