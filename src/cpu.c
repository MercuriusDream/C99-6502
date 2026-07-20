#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "types.h"
#include "instructions_table.h"
#include "addressing.h"
#include "instructions_handlers.h"
#include "stack.h"
#include "trace.h"
#include "interrupt.h"

// Variant-aware instruction dispatch (defined in instructions_handlers.c)
extern void instr_dispatch(void);

// 전역 정의
T_REGISTER REG;
T_BUS BUS;

uint32_t CYCLES;
MEM_WORD OPCODE;
static const INST* META;
SIGNED_MEM_WORD REL_OFFSET;

// CPU variant (defaults to NMOS 6502)
static CPU_VARIANT CPU_VARIANT_MODE = CPU_VARIANT_NMOS_6502;

// CPU state flags for 65C02 WAI and STP instructions
static int CPU_WAITING = 0;
static int CPU_STOPPED = 0;

void cpu_set_variant(CPU_VARIANT variant) {
    CPU_VARIANT_MODE = variant;
}

CPU_VARIANT cpu_get_variant() {
    return CPU_VARIANT_MODE;
}

void cpu_set_waiting(int waiting) {
    CPU_WAITING = waiting;
}

void cpu_set_stopped(int stopped) {
    CPU_STOPPED = stopped;
}

int cpu_is_waiting(void) {
    return CPU_WAITING;
}

int cpu_is_stopped(void) {
    return CPU_STOPPED;
}

static const unsigned char CYCLE_BASE[256] = {
    /* 0x00 .. 0x0F */ 7,6,0,8,3,3,5,5,3,2,2,2,4,4,6,6,
    /* 0x10 .. 0x1F */ 2,5,5,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /* 0x20 .. 0x2F */ 6,6,0,8,3,3,5,5,4,2,2,2,4,4,6,6,
    /* 0x30 .. 0x3F */ 2,5,5,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /* 0x40 .. 0x4F */ 6,6,0,8,3,3,5,5,3,2,2,2,3,4,6,6,
    /* 0x50 .. 0x5F */ 2,5,5,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /* 0x60 .. 0x6F */ 6,6,0,8,3,3,5,5,4,2,2,2,5,4,6,6,
    /* 0x70 .. 0x7F */ 2,5,5,8,4,4,6,6,2,4,2,7,6,4,7,7,
    /* 0x80 .. 0x8F */ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /* 0x90 .. 0x9F */ 2,6,5,6,4,4,4,4,2,5,2,5,5,5,5,5,
    /* 0xA0 .. 0xAF */ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /* 0xB0 .. 0xBF */ 2,5,5,5,4,4,4,4,2,4,2,4,4,4,4,4,
    /* 0xC0 .. 0xCF */ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /* 0xD0 .. 0xDF */ 2,5,5,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /* 0xE0 .. 0xEF */ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /* 0xF0 .. 0xFF */ 2,5,5,8,4,4,6,6,2,4,2,7,4,4,7,7
};

// CMOS 65C02 cycle-count override table.
// Returns the correct 65C02 cycle count for opcodes whose NMOS-slot timing
// in CYCLE_BASE is wrong, or 0 to mean "use CYCLE_BASE".
static int cy65_cycle(MEM_WORD op) {
    switch (op) {
        case 0x04: return 5;
        case 0x0C: return 6;
        case 0x14: return 5;
        case 0x1C: return 6;
        case 0x02: case 0x22: case 0x42: case 0x62: return 2;
        case 0x0F: case 0x1F: case 0x2F: case 0x3F: case 0x4F:
        case 0x5F: case 0x6F: case 0x7F: case 0x8F: case 0x9F:
        case 0xAF: case 0xBF: case 0xCF: case 0xDF: case 0xEF:
        case 0xFF: return 5;
        case 0xCB: return 3;
        case 0xDB: return 3;
        case 0xDA: return 3;
        case 0x5A: return 3;
        case 0xFA: return 4;
        case 0x7A: return 4;
        case 0x9C: return 4;
        case 0xDC: return 5;
        case 0xFC: return 5;
        default:  return 0;
    }
}

// Page-cross +1 must be suppressed for WRITE and RMW indexed ops.
// Their base cycle count is fixed and does not gain a page-cross penalty.
static int op_suppresses_pagecross(MEM_WORD op) {
    switch (op) {
        case 0x9D: /* STA absx */
        case 0x99: /* STA absy */
        case 0x9E: /* STZ absx */
        case 0x91: /* STA indy */
        case 0x1E: /* ASL absx */
        case 0x5E: /* LSR absx */
        case 0x3E: /* ROL absx */
        case 0x7E: /* ROR absx */
        case 0xFE: /* INC absx */
        case 0xDE: /* DEC absx */
            return 1;
        default:
            return 0;
    }
}

// 보정 규칙 적용 함수
static void apply_cycle_adjustments(int page_cross, int branch_taken, int branch_page_cross) {
    if (page_cross && !op_suppresses_pagecross(OPCODE)) CYCLES += 1;
    if (branch_taken) CYCLES += 1;
    if (branch_page_cross) CYCLES += 1;  // 분기 페이지 크로싱 시 추가 +1 (총 +2)
}

MEM_TWO_WORDS EA;
int HAS_EA;
int PAGE_CROSS;
int BRANCH_TAKEN;
int BRANCH_PAGE_CROSS;

void set_zn(MEM_WORD V) {
    if (V == 0) SET_FLAG(FLAG_Z); else CLR_FLAG(FLAG_Z);
    if (V & 0x80) SET_FLAG(FLAG_N); else CLR_FLAG(FLAG_N);
}

void cpu_init() {
    interrupt_init();  // Initialize interrupt controller
}

void cpu_reset() {
    REG.A = 0;
    REG.X = 0;
    REG.Y = 0;
    REG.S = CPU_RESET_STACK_POINTER;
    REG.P = CPU_RESET_STATUS;
    REG.PC = bus_read16(CPU_RESET_VECTOR_ADDRESS);
    cpu_set_waiting(0);
    cpu_set_stopped(0);
    CYCLES += 7;
}

// NMOS addressing-mode overrides for undocumented opcodes.
// The INST_TABLE has CMOS addressing modes; on NMOS many undocumented
// opcodes use different modes.  ADDR_NONE = use the table's mode.
static ADDR_MODE nmos_addr_override(MEM_WORD op) {
    switch (op) {
        // x3 column: (zp,X) even rows, (zp),Y odd rows
        case 0x03: case 0x23: case 0x43: case 0x63: case 0x83: case 0xA3: case 0xC3: case 0xE3:
            return ADDR_INDX;
        case 0x13: case 0x33: case 0x53: case 0x73: case 0xB3: case 0xD3: case 0xF3:
            return ADDR_INDY;
        case 0x93: return ADDR_ABSY;
        // x7 column: zp even rows, zp,X odd rows
        case 0x07: case 0x27: case 0x47: case 0x67: case 0x87: case 0xA7: case 0xC7: case 0xE7:
            return ADDR_ZP;
        case 0x17: case 0x37: case 0x57: case 0x77: case 0xD7: case 0xF7:
            return ADDR_ZPX;
        case 0x97: case 0xB7: return ADDR_ZPY;
        // xB column: #imm even rows, abs,Y odd rows
        case 0x0B: case 0x2B: case 0x4B: case 0x6B: case 0xEB:
            return ADDR_IMM;
        case 0x1B: case 0x3B: case 0x5B: case 0x7B: case 0xDB: case 0xFB:
            return ADDR_ABSY;
        // xF column: abs even rows, abs,X odd rows
        case 0x0F: case 0x2F: case 0x4F: case 0x6F: case 0x8F: case 0xAF: case 0xCF: case 0xEF:
            return ADDR_ABS;
        case 0x1F: case 0x3F: case 0x5F: case 0x7F: case 0xDF: case 0xFF:
            return ADDR_ABSX;
        case 0x9F: case 0xBF: return ADDR_ABSY;
        // x4 NOP variants with different addressing on NMOS
        case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4:
            return ADDR_ZPX;
        case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC:
            return ADDR_ABSX;
        case 0x82: case 0xC2: case 0xE2:
            return ADDR_IMM;
        default:  return ADDR_NONE;
    }
}

// CMOS overrides: opcodes that have operands on NMOS but are 1-byte NOPs on CMOS.
// xB column (except $CB=WAI, $DB=STP which are real CMOS instructions).
static int cmos_addr_override(MEM_WORD op, ADDR_MODE* out) {
    switch (op) {
        case 0x0B: case 0x1B: case 0x2B: case 0x3B:
        case 0x4B: case 0x5B: case 0x6B: case 0x7B:
        case 0x8B: case 0x9B: case 0xAB: case 0xBB:
        case 0xEB: case 0xFB:
            *out = ADDR_NONE; return 1;
        default:  return 0;
    }
}

void cpu_step() {
    INTERRUPT_TYPE t = interrupt_poll();
    if (t != INT_NONE) {
        interrupt_begin(t);
        interrupt_step_cycle();
        return;
    }

    if (CPU_STOPPED) return;
    if (CPU_WAITING) return;

    MEM_TWO_WORDS pc_before = REG.PC;
    OPCODE = bus_read(REG.PC++);
    META = &INST_TABLE[OPCODE>>4][OPCODE&0x0F];

    trace_before(pc_before, OPCODE);

    if (cpu_get_variant() == CPU_VARIANT_CMOS_65C02) {
        int c = cy65_cycle(OPCODE);
        CYCLES += c ? c : CYCLE_BASE[OPCODE];
    } else {
        CYCLES += CYCLE_BASE[OPCODE];
    }

    // Determine the correct addressing mode for this variant
    ADDR_MODE mode = META->ADDR;
    if (cpu_get_variant() == CPU_VARIANT_NMOS_6502) {
        ADDR_MODE ov = nmos_addr_override(OPCODE);
        if (ov != ADDR_NONE) mode = ov;
    } else {
        ADDR_MODE ov;
        if (cmos_addr_override(OPCODE, &ov)) mode = ov;
    }

    HAS_EA = 1;
    EA = addr_resolve(mode, &PAGE_CROSS, &HAS_EA);
    BRANCH_TAKEN = 0;
    BRANCH_PAGE_CROSS = 0;

    instr_dispatch();

    apply_cycle_adjustments(PAGE_CROSS, BRANCH_TAKEN, BRANCH_PAGE_CROSS);

    trace_after();
}

void cpu_run(uint32_t MAX_CYCLES) {
    uint32_t start_cycles = CYCLES;
    while (CYCLES - start_cycles < MAX_CYCLES) {
        cpu_step();
    }
}

void cpu_irq() {
    // Assert IRQ line (level-triggered); serviced at next instruction boundary.
    interrupt_set_irq(1);
}

void cpu_nmi() {
    // Assert then deassert NMI line to create a falling edge; serviced at next
    // instruction boundary.
    interrupt_set_nmi(1);
    interrupt_set_nmi(0);
}

MEM_WORD fetch8() {
    return bus_read(REG.PC++);
}

MEM_TWO_WORDS fetch16() {
    MEM_WORD LO = bus_read(REG.PC);
    MEM_WORD HI = bus_read(REG.PC + 1);
    REG.PC += 2;
    return (MEM_TWO_WORDS)(LO | ((MEM_TWO_WORDS)HI << 8));
}
