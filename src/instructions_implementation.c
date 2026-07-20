#include "instructions_implementation.h"
#include "types.h"
#include "bus.h"
#include "cpu.h"
#include "stack.h"
#include "interrupt.h"
#include <stdio.h>

extern MEM_TWO_WORDS EA;
extern int HAS_EA;
extern int BRANCH_TAKEN;
extern int BRANCH_PAGE_CROSS;

void LDA(void) { REG.A = HAS_EA ? bus_read(EA) : (MEM_WORD)EA; set_zn(REG.A); }
void STA(void) { bus_write(EA, REG.A); }

void BPL_(void) { if (!GET_FLAG(FLAG_N)) { MEM_TWO_WORDS old_pc = REG.PC; REG.PC = EA; BRANCH_TAKEN = 1; BRANCH_PAGE_CROSS = ((old_pc & 0xFF00) != (REG.PC & 0xFF00)); } }
void BMI_(void) { if (GET_FLAG(FLAG_N)) { MEM_TWO_WORDS old_pc = REG.PC; REG.PC = EA; BRANCH_TAKEN = 1; BRANCH_PAGE_CROSS = ((old_pc & 0xFF00) != (REG.PC & 0xFF00)); } }
void BVC_(void) { if (!GET_FLAG(FLAG_V)) { MEM_TWO_WORDS old_pc = REG.PC; REG.PC = EA; BRANCH_TAKEN = 1; BRANCH_PAGE_CROSS = ((old_pc & 0xFF00) != (REG.PC & 0xFF00)); } }
void BVS_(void) { if (GET_FLAG(FLAG_V)) { MEM_TWO_WORDS old_pc = REG.PC; REG.PC = EA; BRANCH_TAKEN = 1; BRANCH_PAGE_CROSS = ((old_pc & 0xFF00) != (REG.PC & 0xFF00)); } }
void BCC_(void) { if (!GET_FLAG(FLAG_C)) { MEM_TWO_WORDS old_pc = REG.PC; REG.PC = EA; BRANCH_TAKEN = 1; BRANCH_PAGE_CROSS = ((old_pc & 0xFF00) != (REG.PC & 0xFF00)); } }
void BCS_(void) { if (GET_FLAG(FLAG_C)) { MEM_TWO_WORDS old_pc = REG.PC; REG.PC = EA; BRANCH_TAKEN = 1; BRANCH_PAGE_CROSS = ((old_pc & 0xFF00) != (REG.PC & 0xFF00)); } }
void BNE_(void) { if (!GET_FLAG(FLAG_Z)) { MEM_TWO_WORDS old_pc = REG.PC; REG.PC = EA; BRANCH_TAKEN = 1; BRANCH_PAGE_CROSS = ((old_pc & 0xFF00) != (REG.PC & 0xFF00)); } }
void BEQ_(void) { if (GET_FLAG(FLAG_Z)) { MEM_TWO_WORDS old_pc = REG.PC; REG.PC = EA; BRANCH_TAKEN = 1; BRANCH_PAGE_CROSS = ((old_pc & 0xFF00) != (REG.PC & 0xFF00)); } }

void LDX(void) { REG.X = HAS_EA ? bus_read(EA) : (MEM_WORD)EA; set_zn(REG.X); }
void LDY(void) { REG.Y = HAS_EA ? bus_read(EA) : (MEM_WORD)EA; set_zn(REG.Y); }
void STX(void) { bus_write(EA, REG.X); }
void STY(void) { bus_write(EA, REG.Y); }

void TAX(void) { REG.X = REG.A; set_zn(REG.X); }
void TXA(void) { REG.A = REG.X; set_zn(REG.A); }
void TAY(void) { REG.Y = REG.A; set_zn(REG.Y); }
void TYA(void) { REG.A = REG.Y; set_zn(REG.A); }
void TSX(void) { REG.X = REG.S; set_zn(REG.X); }
void TXS(void) { REG.S = REG.X; }
void PHA(void) { push8(REG.A); }
void PLA(void) { REG.A = pop8(); set_zn(REG.A); }
void PHP(void) { push8(REG.P | FLAG_B | FLAG_U); }
void PLP(void) { REG.P = (pop8() & ~FLAG_B) | FLAG_U; }

void INX(void) { REG.X = (REG.X + 1) & 0xFF; set_zn(REG.X); }
void DEX(void) { REG.X = (REG.X - 1) & 0xFF; set_zn(REG.X); }
void INY(void) { REG.Y = (REG.Y + 1) & 0xFF; set_zn(REG.Y); }
void DEY(void) { REG.Y = (REG.Y - 1) & 0xFF; set_zn(REG.Y); }
void INC(void) { MEM_WORD v = (bus_read(EA) + 1) & 0xFF; bus_write(EA, v); set_zn(v); }
void DEC(void) { MEM_WORD v = (bus_read(EA) - 1) & 0xFF; bus_write(EA, v); set_zn(v); }

// 65C02 INC A / DEC A (accumulator forms of $1A / $3A)
void INC_A(void) { REG.A = (REG.A + 1) & 0xFF; set_zn(REG.A); }
void DEC_A(void) { REG.A = (REG.A - 1) & 0xFF; set_zn(REG.A); }

void AND(void) { MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA; REG.A &= data; set_zn(REG.A); }
void ORA(void) { MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA; REG.A |= data; set_zn(REG.A); }
void EOR(void) { MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA; REG.A ^= data; set_zn(REG.A); }

static inline void adc_core(MEM_WORD v) {
    if (!GET_FLAG(FLAG_D)) {
        // Binary mode
        unsigned a = REG.A;
        unsigned c = GET_FLAG(FLAG_C) ? 1u : 0u;
        unsigned sum = a + v + c;

        REG.A = (MEM_WORD)(sum & 0xFF);

        // Carry: set if result > 0xFF
        if (sum > 0xFF) SET_FLAG(FLAG_C); else CLR_FLAG(FLAG_C);

        // Overflow: set if sign bit is incorrect
        if ((~(a ^ v) & (a ^ sum) & 0x80) != 0) SET_FLAG(FLAG_V); else CLR_FLAG(FLAG_V);

        set_zn(REG.A);
    } else {
        // BCD mode (hardware-accurate for NMOS 6502)
        unsigned a = REG.A;
        unsigned c = GET_FLAG(FLAG_C) ? 1u : 0u;
        unsigned binary_sum = a + v + c;  // Binary result for N/Z flags (NMOS behavior)

        unsigned al = (a & 0x0F) + (v & 0x0F) + c;
        unsigned ah = (a >> 4) + (v >> 4);

        // Adjust low nibble if needed
        if (al >= 0x0A) {
            al = ((al + 0x06) & 0x0F);
            ah++;
        }

        // Adjust high nibble if needed
        if (ah >= 0x0A) {
            ah += 0x06;
        }

        // Carry is set if high nibble overflowed
        if (ah >= 0x10) SET_FLAG(FLAG_C); else CLR_FLAG(FLAG_C);

        REG.A = (MEM_WORD)(((ah & 0x0F) << 4) | (al & 0x0F));

        // NMOS 6502: N and Z flags set from binary result (before BCD adjustment)
        // CMOS 65C02: N and Z flags set from BCD result (after adjustment)
        if (cpu_get_variant() == CPU_VARIANT_CMOS_65C02) {
            set_zn(REG.A);  // Use adjusted BCD result
        } else {
            set_zn((MEM_WORD)(binary_sum & 0xFF));  // Use binary result
        }

        // V flag: compute from binary operation
        if ((~(a ^ v) & (a ^ binary_sum) & 0x80) != 0) SET_FLAG(FLAG_V); else CLR_FLAG(FLAG_V);
    }
}

static inline void sbc_core(MEM_WORD v) {
    if (!GET_FLAG(FLAG_D)) {
        // Binary mode: A = A - M - (1 - C) = A + ~M + C
        unsigned a = REG.A;
        unsigned c = GET_FLAG(FLAG_C) ? 1u : 0u;
        unsigned sub = a + (v ^ 0xFF) + c;

        REG.A = (MEM_WORD)(sub & 0xFF);

        // Carry: set if no borrow (A >= M in unsigned comparison)
        if (sub > 0xFF) SET_FLAG(FLAG_C); else CLR_FLAG(FLAG_C);

        // Overflow: set if sign bit is incorrect
        if (((a ^ v) & (a ^ sub) & 0x80) != 0) SET_FLAG(FLAG_V); else CLR_FLAG(FLAG_V);

        set_zn(REG.A);
    } else {
        // BCD mode subtraction (hardware-accurate for NMOS 6502)
        unsigned a = REG.A;
        unsigned c = GET_FLAG(FLAG_C) ? 1u : 0u;
        unsigned binary_sub = a + (v ^ 0xFF) + c;  // Binary result for N/Z/V flags

        // BCD subtraction with borrow
        int al = (int)(a & 0x0F) - (int)(v & 0x0F) - (c ? 0 : 1);
        int ah = (int)(a >> 4) - (int)(v >> 4);

        // Adjust low nibble if needed (borrow from high nibble)
        if (al < 0) {
            al = ((al - 0x06) & 0x0F);
            ah--;
        }

        // Adjust high nibble if needed
        if (ah < 0) {
            ah = ((ah - 0x06) & 0x0F);
        }

        // Carry: set if no borrow occurred (binary_sub didn't need a borrow)
        if (binary_sub > 0xFF) SET_FLAG(FLAG_C); else CLR_FLAG(FLAG_C);

        REG.A = (MEM_WORD)(((ah & 0x0F) << 4) | (al & 0x0F));

        // NMOS 6502: N and Z flags set from binary result
        // CMOS 65C02: N and Z flags set from BCD result (after adjustment)
        if (cpu_get_variant() == CPU_VARIANT_CMOS_65C02) {
            set_zn(REG.A);  // Use adjusted BCD result
        } else {
            set_zn((MEM_WORD)(binary_sub & 0xFF));  // Use binary result
        }

        // V flag: compute from binary operation
        if (((a ^ v) & (a ^ binary_sub) & 0x80) != 0) SET_FLAG(FLAG_V); else CLR_FLAG(FLAG_V);
    }
}

void ADC(void) {
    MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA;
    adc_core(data);
}

void SBC(void) {
    MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA;
    sbc_core(data);
}

void CLC(void) { CLR_FLAG(FLAG_C); }
void SEC(void) { SET_FLAG(FLAG_C); }
void CLI(void) { CLR_FLAG(FLAG_I); }
void SEI(void) { SET_FLAG(FLAG_I); }
void CLD(void) { CLR_FLAG(FLAG_D); }
void SED(void) { SET_FLAG(FLAG_D); }
void CLV(void) { CLR_FLAG(FLAG_V); }
void NOP(void) { }
void BRK(void) {
    // BRK software interrupt
    // Note: PC is already pointing to the byte after BRK opcode when this handler executes
    // The interrupt controller will handle the full sequence
    interrupt_begin(INT_BRK);
    interrupt_step_cycle();  // Execute interrupt sequence
}

void JMP(void) {
    REG.PC = EA;
    if (cpu_get_variant() == CPU_VARIANT_CMOS_65C02 && OPCODE == 0x6C) {
        CYCLES += 1;
    }
}
void JSR(void) { push16(REG.PC - 1); REG.PC = EA; }
void RTS(void) { REG.PC = pop16() + 1; }
void RTI(void) { REG.P = pop8() & ~(FLAG_B | FLAG_U); REG.PC = pop16(); }

void ASL(void) {
    if (!HAS_EA) {
        CLR_FLAG(FLAG_C);
        if (REG.A & 0x80) SET_FLAG(FLAG_C);
        REG.A = (REG.A << 1) & 0xFF;
        set_zn(REG.A);
    } else {
        MEM_WORD v = bus_read(EA);
        CLR_FLAG(FLAG_C);
        if (v & 0x80) SET_FLAG(FLAG_C);
        v = (v << 1) & 0xFF;
        bus_write(EA, v);
        set_zn(v);
    }
}

void LSR(void) {
    if (!HAS_EA) {
        CLR_FLAG(FLAG_C);
        if (REG.A & 0x01) SET_FLAG(FLAG_C);
        REG.A >>= 1;
        set_zn(REG.A);
    } else {
        MEM_WORD v = bus_read(EA);
        CLR_FLAG(FLAG_C);
        if (v & 0x01) SET_FLAG(FLAG_C);
        v >>= 1;
        bus_write(EA, v);
        set_zn(v);
    }
}

void ROL(void) {
    int carry = GET_FLAG(FLAG_C);
    if (!HAS_EA) {
        CLR_FLAG(FLAG_C);
        if (REG.A & 0x80) SET_FLAG(FLAG_C);
        REG.A = ((REG.A << 1) & 0xFF) | carry;
        set_zn(REG.A);
    } else {
        MEM_WORD v = bus_read(EA);
        CLR_FLAG(FLAG_C);
        if (v & 0x80) SET_FLAG(FLAG_C);
        v = ((v << 1) & 0xFF) | carry;
        bus_write(EA, v);
        set_zn(v);
    }
}

void ROR(void) {
    int carry = GET_FLAG(FLAG_C);
    if (!HAS_EA) {
        CLR_FLAG(FLAG_C);
        if (REG.A & 0x01) SET_FLAG(FLAG_C);
        REG.A = (REG.A >> 1) | (carry << 7);
        set_zn(REG.A);
    } else {
        MEM_WORD v = bus_read(EA);
        CLR_FLAG(FLAG_C);
        if (v & 0x01) SET_FLAG(FLAG_C);
        v = (v >> 1) | (carry << 7);
        bus_write(EA, v);
        set_zn(v);
    }
}

void CMP(void) {
    MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA;
    if (REG.A >= data) SET_FLAG(FLAG_C); else CLR_FLAG(FLAG_C);
    set_zn((MEM_WORD)(REG.A - data));
}

void CPX(void) {
    MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA;
    if (REG.X >= data) SET_FLAG(FLAG_C); else CLR_FLAG(FLAG_C);
    set_zn((MEM_WORD)(REG.X - data));
}

void CPY(void) {
    MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA;
    if (REG.Y >= data) SET_FLAG(FLAG_C); else CLR_FLAG(FLAG_C);
    set_zn((MEM_WORD)(REG.Y - data));
}

void BIT(void) {
    MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA;
    MEM_WORD result = REG.A & data;
    if (result == 0) SET_FLAG(FLAG_Z); else CLR_FLAG(FLAG_Z);
    if (OPCODE != 0x89) {
        if (data & 0x80) SET_FLAG(FLAG_N); else CLR_FLAG(FLAG_N);
        if (data & 0x40) SET_FLAG(FLAG_V); else CLR_FLAG(FLAG_V);
    }
}

// Undocumented opcodes

// LAX - Load A and X (LDA + LDX combined)
void LAX(void) {
    MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA;
    REG.A = data;
    REG.X = data;
    set_zn(REG.A);
}

// SAX - Store A AND X (bitwise AND of A and X)
void SAX(void) {
    bus_write(EA, REG.A & REG.X);
}

// DCP - Decrement memory then Compare with A (DEC + CMP)
void DCP(void) {
    MEM_WORD v = (bus_read(EA) - 1) & 0xFF;
    bus_write(EA, v);
    if (REG.A >= v) SET_FLAG(FLAG_C); else CLR_FLAG(FLAG_C);
    set_zn((MEM_WORD)(REG.A - v));
}

// ISC (ISB) - Increment memory then SBC (INC + SBC)
void ISC(void) {
    MEM_WORD v = (bus_read(EA) + 1) & 0xFF;
    bus_write(EA, v);
    sbc_core(v);
}

// SLO - Shift Left then OR (ASL + ORA)
void SLO(void) {
    MEM_WORD v = bus_read(EA);
    CLR_FLAG(FLAG_C);
    if (v & 0x80) SET_FLAG(FLAG_C);
    v = (v << 1) & 0xFF;
    bus_write(EA, v);
    REG.A |= v;
    set_zn(REG.A);
}

// RLA - Rotate Left then AND (ROL + AND)
void RLA(void) {
    int carry = GET_FLAG(FLAG_C);
    MEM_WORD v = bus_read(EA);
    CLR_FLAG(FLAG_C);
    if (v & 0x80) SET_FLAG(FLAG_C);
    v = ((v << 1) & 0xFF) | carry;
    bus_write(EA, v);
    REG.A &= v;
    set_zn(REG.A);
}

// SRE - Shift Right then EOR (LSR + EOR)
void SRE(void) {
    MEM_WORD v = bus_read(EA);
    CLR_FLAG(FLAG_C);
    if (v & 0x01) SET_FLAG(FLAG_C);
    v >>= 1;
    bus_write(EA, v);
    REG.A ^= v;
    set_zn(REG.A);
}

// RRA - Rotate Right then ADC (ROR + ADC)
void RRA(void) {
    int carry = GET_FLAG(FLAG_C);
    MEM_WORD v = bus_read(EA);
    CLR_FLAG(FLAG_C);
    if (v & 0x01) SET_FLAG(FLAG_C);
    v = (v >> 1) | (carry << 7);
    bus_write(EA, v);
    adc_core(v);
}

// ANC - AND operand with A, then copy bit 7 to Carry (immediate-mode illegal ops $0B/$2B)
void ANC(void) {
    MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA;
    REG.A &= data;
    set_zn(REG.A);
    if (REG.A & 0x80) SET_FLAG(FLAG_C); else CLR_FLAG(FLAG_C);
}

// ALR (ASR) - AND operand with A, then LSR A (illegal op $4B)
void ALR(void) {
    MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA;
    REG.A &= data;
    CLR_FLAG(FLAG_C);
    if (REG.A & 0x01) SET_FLAG(FLAG_C);
    REG.A >>= 1;
    set_zn(REG.A);
}

// ARR - AND operand with A, then ROR A (illegal op $6B)
// On NMOS the flags are subtle; we implement the commonly-documented behavior:
// A = (A & imm) ROR 1; C = bit6 of result, V = bit6 XOR bit5.
void ARR(void) {
    MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA;
    REG.A &= data;
    int carry = GET_FLAG(FLAG_C);
    REG.A = (REG.A >> 1) | (carry << 7);
    set_zn(REG.A);
    // Carry = bit 6 of result
    if (REG.A & 0x40) SET_FLAG(FLAG_C); else CLR_FLAG(FLAG_C);
    // Overflow = bit6 XOR bit5
    if (((REG.A >> 6) ^ (REG.A >> 5)) & 1) SET_FLAG(FLAG_V); else CLR_FLAG(FLAG_V);
}

// ALT_SBC (USBC) - alternate encoding of SBC ($EB). Behaves identically to SBC.
void ALT_SBC(void) {
    MEM_WORD data = HAS_EA ? bus_read(EA) : (MEM_WORD)EA;
    sbc_core(data);
}

// KIL (JAM) - halt the CPU permanently (illegal ops $02/$12/$22/$32/$42/$52/$62/$72/$92/$B2/$D2/$F2)
void op_kil(void) {
    cpu_set_stopped(1);
}

// Illegal NOP variants (read but do nothing)
void NOP_READ(void) {
    if (HAS_EA) {
        bus_read(EA);  // Read memory but ignore result
    }
}

// CMOS 65C02 Instructions
void BRA_(void) {
    MEM_TWO_WORDS
    old_pc = REG.PC;
    REG.PC = EA;
    BRANCH_TAKEN = 1;
    BRANCH_PAGE_CROSS = ((old_pc & 0xFF00) != (REG.PC & 0xFF00));
}

void PHX(void) { push8(REG.X); }
void PHY(void) { push8(REG.Y); }

void PLX(void) {
    REG.X = pop8();
    set_zn(REG.X);
}
void PLY(void) {
    REG.Y = pop8();
    set_zn(REG.Y);
}

void STZ(void) { bus_write(EA, 0x00); }

void TRB(void) {
    MEM_WORD data = bus_read(EA);
    MEM_WORD result = REG.A & data;
    if (!(result)) SET_FLAG(FLAG_Z);
    else CLR_FLAG(FLAG_Z);
    bus_write(EA, data & ~REG.A);
}
void TSB(void) {
    MEM_WORD data = bus_read(EA);
    MEM_WORD result = REG.A & data;
    if (!(result)) SET_FLAG(FLAG_Z);
    else CLR_FLAG(FLAG_Z);
    bus_write(EA, data | REG.A);
}

void WAI(void) { cpu_set_waiting(1); }
void STP(void) { cpu_set_stopped(1); }

// Rockwell/WDC 65C02 Bit Manipulation Instructions
void RMB(void) {
    MEM_WORD bit = (OPCODE >> 4) & 0x07;
    MEM_WORD data = bus_read(EA);
    bus_write(EA, data & ~(1 << bit));
}
void SMB(void) {
    MEM_WORD bit = (OPCODE >> 4) & 0x07;
    MEM_WORD data = bus_read(EA);
    bus_write(EA, data | (1 << bit));
}
void BBR_(void) {
    MEM_WORD bit = (OPCODE >> 4) & 0x07;
    MEM_WORD data = bus_read(EA);
    if (!(data & (1<<bit))) {
        MEM_TWO_WORDS old_pc = REG.PC;
        REG.PC = (REG.PC+REL_OFFSET) & 0xFFFF;
        BRANCH_TAKEN = 1;
        BRANCH_PAGE_CROSS = ((old_pc & 0xFF00) != (REG.PC & 0xFF00));
    }
}
void BBS_(void) {
    MEM_WORD bit = (OPCODE >> 4) & 0x07;
    MEM_WORD data = bus_read(EA);
    if ((data & (1<<bit))) {
        MEM_TWO_WORDS old_pc = REG.PC;
        REG.PC = (REG.PC + REL_OFFSET) & 0xFFFF;
        BRANCH_TAKEN = 1;
        BRANCH_PAGE_CROSS = ((old_pc & 0xFF00) != (REG.PC & 0xFF00));
    }
}