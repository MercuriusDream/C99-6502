#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "types.h"
#include "instruments_table.h"
#include "addressing.h"
#include "instruments_handlers.h"
#include "stack.h"
#include "trace.h"
#include "interrupt.h"

extern const instr_fn INSTR_HANDLERS[256];

// 전역 정의
T_REGISTER REG;
T_BUS BUS;

static MEM_TWO_WORDS CYCLES;
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
    /* 0x10 .. 0x1F */ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /* 0x20 .. 0x2F */ 6,6,0,8,3,3,5,5,4,2,2,2,4,4,6,6,
    /* 0x30 .. 0x3F */ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /* 0x40 .. 0x4F */ 6,6,0,8,3,3,5,5,3,2,2,2,3,4,6,6,
    /* 0x50 .. 0x5F */ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /* 0x60 .. 0x6F */ 6,6,0,8,3,3,5,5,4,2,2,2,5,4,6,6,
    /* 0x70 .. 0x7F */ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /* 0x80 .. 0x8F */ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /* 0x90 .. 0x9F */ 2,6,0,6,4,4,4,4,2,5,2,5,5,5,5,5,
    /* 0xA0 .. 0xAF */ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /* 0xB0 .. 0xBF */ 2,5,0,5,4,4,4,4,2,4,2,4,4,4,4,4,
    /* 0xC0 .. 0xCF */ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /* 0xD0 .. 0xDF */ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /* 0xE0 .. 0xEF */ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /* 0xF0 .. 0xFF */ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7
};

// 보정 규칙 적용 함수
static void apply_cycle_adjustments(int page_cross, int branch_taken, int branch_page_cross) {
    if (page_cross) CYCLES += 1;
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
    REG.S = CPU_RESET_STACK_POINTER;
    REG.P = CPU_RESET_STATUS;
    REG.PC = bus_read16(CPU_RESET_VECTOR_ADDRESS);
}

void cpu_step() {
    if (CPU_STOPPED) return;
    if (CPU_WAITING) return;

    MEM_TWO_WORDS pc_before = REG.PC;
    OPCODE = bus_read(REG.PC++);
    META = &INST_TABLE[OPCODE>>4][OPCODE&0x0F];

    trace_before(pc_before, OPCODE);

    CYCLES += CYCLE_BASE[OPCODE];

    HAS_EA = 1;
    EA = addr_resolve(META->ADDR, &PAGE_CROSS, &HAS_EA);
    BRANCH_TAKEN = 0;
    BRANCH_PAGE_CROSS = 0;

    INSTR_HANDLERS[OPCODE]();

    apply_cycle_adjustments(PAGE_CROSS, BRANCH_TAKEN, BRANCH_PAGE_CROSS);

    trace_after();
}

void cpu_run(MEM_TWO_WORDS MAX_CYCLES) {
    MEM_TWO_WORDS start_cycles = CYCLES;
    while (CYCLES - start_cycles < MAX_CYCLES) {
        cpu_step();
    }
}

void cpu_irq() {
    // Trigger IRQ via interrupt controller
    interrupt_set_irq(1);  // Assert IRQ line
    INTERRUPT_TYPE type = interrupt_poll();
    if (type == INT_IRQ) {
        interrupt_begin(INT_IRQ);
        interrupt_step_cycle();
    }
}

void cpu_nmi() {
    // Trigger NMI via interrupt controller (edge-triggered)
    interrupt_set_nmi(1);  // Assert NMI line (high)
    interrupt_set_nmi(0);  // Deassert to create falling edge
    INTERRUPT_TYPE type = interrupt_poll();
    if (type == INT_NMI) {
        interrupt_begin(INT_NMI);
        interrupt_step_cycle();
    }
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
