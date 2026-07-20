#include "types.h"
#include "instructions_handlers.h"
#include "instructions_implementation.h"
#include "cpu.h"

void op_illegal(void) {
    // Illegal/unimplemented opcode - treated as NOP for stability
}

// ---------------------------------------------------------------------------
// CMOS 65C02 handler table (base table).
// Official opcodes + all 65C02 additions (TSB/TRB/RMB/SMB/BBR/BBS/BRA/PHX/PHY/
// PLX/PLY/STZ/WAI/STP/INC_A/DEC_A).  Formerly-undocumented NMOS slots are NOP.
// ---------------------------------------------------------------------------
const instr_fn INSTR_HANDLERS[256] = {
    // 0x0_
    [0x00] = BRK,      [0x01] = ORA,      [0x02] = NOP,        [0x03] = NOP,
    [0x04] = TSB,      [0x05] = ORA,      [0x06] = ASL,        [0x07] = RMB,
    [0x08] = PHP,      [0x09] = ORA,      [0x0A] = ASL,        [0x0B] = NOP,
    [0x0C] = TSB,      [0x0D] = ORA,      [0x0E] = ASL,        [0x0F] = BBR_,

    // 0x1_
    [0x10] = BPL_,     [0x11] = ORA,      [0x12] = ORA,        [0x13] = NOP,
    [0x14] = TRB,      [0x15] = ORA,      [0x16] = ASL,        [0x17] = RMB,
    [0x18] = CLC,      [0x19] = ORA,      [0x1A] = INC_A,      [0x1B] = NOP,
    [0x1C] = TRB,      [0x1D] = ORA,      [0x1E] = ASL,        [0x1F] = BBR_,

    // 0x2_
    [0x20] = JSR,      [0x21] = AND,      [0x22] = NOP,        [0x23] = NOP,
    [0x24] = BIT,      [0x25] = AND,      [0x26] = ROL,        [0x27] = RMB,
    [0x28] = PLP,      [0x29] = AND,      [0x2A] = ROL,        [0x2B] = NOP,
    [0x2C] = BIT,      [0x2D] = AND,      [0x2E] = ROL,        [0x2F] = BBR_,

    // 0x3_
    [0x30] = BMI_,     [0x31] = AND,      [0x32] = AND,        [0x33] = NOP,
    [0x34] = BIT,       [0x35] = AND,      [0x36] = ROL,        [0x37] = RMB,
    [0x38] = SEC,      [0x39] = AND,      [0x3A] = DEC_A,      [0x3B] = NOP,
    [0x3C] = BIT,       [0x3D] = AND,      [0x3E] = ROL,        [0x3F] = BBR_,

    // 0x4_
    [0x40] = RTI,      [0x41] = EOR,      [0x42] = NOP,        [0x43] = NOP,
    [0x44] = NOP_READ, [0x45] = EOR,      [0x46] = LSR,        [0x47] = RMB,
    [0x48] = PHA,      [0x49] = EOR,      [0x4A] = LSR,        [0x4B] = NOP,
    [0x4C] = JMP,      [0x4D] = EOR,      [0x4E] = LSR,        [0x4F] = BBR_,

    // 0x5_
    [0x50] = BVC_,     [0x51] = EOR,      [0x52] = EOR,        [0x53] = NOP,
    [0x54] = NOP_READ, [0x55] = EOR,      [0x56] = LSR,        [0x57] = RMB,
    [0x58] = CLI,      [0x59] = EOR,      [0x5A] = PHY,        [0x5B] = NOP,
    [0x5C] = NOP_READ, [0x5D] = EOR,      [0x5E] = LSR,        [0x5F] = BBR_,

    // 0x6_
    [0x60] = RTS,      [0x61] = ADC,      [0x62] = NOP,        [0x63] = NOP,
    [0x64] = STZ,      [0x65] = ADC,      [0x66] = ROR,        [0x67] = RMB,
    [0x68] = PLA,      [0x69] = ADC,      [0x6A] = ROR,        [0x6B] = NOP,
    [0x6C] = JMP,      [0x6D] = ADC,      [0x6E] = ROR,        [0x6F] = BBR_,

    // 0x7_
    [0x70] = BVS_,     [0x71] = ADC,      [0x72] = ADC,        [0x73] = NOP,
    [0x74] = STZ,      [0x75] = ADC,      [0x76] = ROR,        [0x77] = RMB,
    [0x78] = SEI,      [0x79] = ADC,      [0x7A] = PLY,        [0x7B] = NOP,
    [0x7C] = JMP,      [0x7D] = ADC,      [0x7E] = ROR,        [0x7F] = BBR_,

    // 0x8_
    [0x80] = BRA_,     [0x81] = STA,      [0x82] = NOP,        [0x83] = NOP,
    [0x84] = STY,      [0x85] = STA,      [0x86] = STX,        [0x87] = SMB,
    [0x88] = DEY,      [0x89] = BIT,      [0x8A] = TXA,        [0x8B] = NOP,
    [0x8C] = STY,      [0x8D] = STA,      [0x8E] = STX,        [0x8F] = BBS_,

    // 0x9_
    [0x90] = BCC_,     [0x91] = STA,      [0x92] = STA,        [0x93] = NOP,
    [0x94] = STY,      [0x95] = STA,      [0x96] = STX,        [0x97] = SMB,
    [0x98] = TYA,      [0x99] = STA,      [0x9A] = TXS,        [0x9B] = NOP,
    [0x9C] = STZ,      [0x9D] = STA,      [0x9E] = STZ,        [0x9F] = BBS_,

    // 0xA_
    [0xA0] = LDY,      [0xA1] = LDA,      [0xA2] = LDX,        [0xA3] = NOP,
    [0xA4] = LDY,      [0xA5] = LDA,      [0xA6] = LDX,        [0xA7] = SMB,
    [0xA8] = TAY,      [0xA9] = LDA,      [0xAA] = TAX,        [0xAB] = NOP,
    [0xAC] = LDY,      [0xAD] = LDA,      [0xAE] = LDX,        [0xAF] = BBS_,

    // 0xB_
    [0xB0] = BCS_,     [0xB1] = LDA,      [0xB2] = LDA,        [0xB3] = NOP,
    [0xB4] = LDY,      [0xB5] = LDA,      [0xB6] = LDX,        [0xB7] = SMB,
    [0xB8] = CLV,      [0xB9] = LDA,      [0xBA] = TSX,        [0xBB] = NOP,
    [0xBC] = LDY,      [0xBD] = LDA,      [0xBE] = LDX,        [0xBF] = BBS_,

    // 0xC_
    [0xC0] = CPY,      [0xC1] = CMP,      [0xC2] = NOP,        [0xC3] = NOP,
    [0xC4] = CPY,      [0xC5] = CMP,      [0xC6] = DEC,        [0xC7] = SMB,
    [0xC8] = INY,      [0xC9] = CMP,      [0xCA] = DEX,        [0xCB] = WAI,
    [0xCC] = CPY,      [0xCD] = CMP,      [0xCE] = DEC,        [0xCF] = BBS_,

    // 0xD_
    [0xD0] = BNE_,     [0xD1] = CMP,      [0xD2] = CMP,        [0xD3] = NOP,
    [0xD4] = NOP_READ, [0xD5] = CMP,      [0xD6] = DEC,        [0xD7] = SMB,
    [0xD8] = CLD,      [0xD9] = CMP,      [0xDA] = PHX,        [0xDB] = STP,
    [0xDC] = NOP_READ, [0xDD] = CMP,      [0xDE] = DEC,        [0xDF] = BBS_,

    // 0xE_
    [0xE0] = CPX,      [0xE1] = SBC,      [0xE2] = NOP,        [0xE3] = NOP,
    [0xE4] = CPX,      [0xE5] = SBC,      [0xE6] = INC,        [0xE7] = SMB,
    [0xE8] = INX,      [0xE9] = SBC,      [0xEA] = NOP,        [0xEB] = NOP,
    [0xEC] = CPX,      [0xED] = SBC,      [0xEE] = INC,        [0xEF] = BBS_,

    // 0xF_
    [0xF0] = BEQ_,     [0xF1] = SBC,      [0xF2] = SBC,        [0xF3] = NOP,
    [0xF4] = NOP_READ, [0xF5] = SBC,      [0xF6] = INC,        [0xF7] = SMB,
    [0xF8] = SED,      [0xF9] = SBC,      [0xFA] = PLX,        [0xFB] = NOP,
    [0xFC] = NOP_READ, [0xFD] = SBC,      [0xFE] = INC,        [0xFF] = BBS_
};

// ---------------------------------------------------------------------------
// NMOS 6502 overrides — opcodes that differ from the CMOS table above.
// NULL = use the CMOS base handler (official opcodes are identical).
// ---------------------------------------------------------------------------
static const instr_fn NMOS_HANDLER[256] = {
    // x2 column: KIL on NMOS (NOP on CMOS)
    [0x02] = op_kil, [0x12] = op_kil, [0x22] = op_kil, [0x32] = op_kil,
    [0x42] = op_kil, [0x52] = op_kil, [0x62] = op_kil, [0x72] = op_kil,
    [0x92] = op_kil, [0xB2] = op_kil, [0xD2] = op_kil, [0xF2] = op_kil,

    // x3 column: undocumented on NMOS (NOP on CMOS)
    [0x03] = SLO, [0x13] = SLO, [0x23] = RLA, [0x33] = RLA,
    [0x43] = SRE, [0x53] = SRE, [0x63] = RRA, [0x73] = RRA,
    [0x83] = SAX, [0x93] = NOP_READ, [0xA3] = LAX, [0xB3] = LAX,
    [0xC3] = DCP, [0xD3] = DCP, [0xE3] = ISC, [0xF3] = ISC,

    // x4 column: NOP on NMOS (TSB/TRB/STZ on CMOS)
    [0x04] = NOP_READ, [0x0C] = NOP_READ, [0x14] = NOP_READ,
    [0x1C] = NOP_READ, [0x64] = NOP_READ, [0x74] = NOP_READ,
    [0x34] = NOP_READ, [0x3C] = NOP_READ,

    // x7 column: undocumented on NMOS (RMB/SMB on CMOS)
    [0x07] = SLO, [0x17] = SLO, [0x27] = RLA, [0x37] = RLA,
    [0x47] = SRE, [0x57] = SRE, [0x67] = RRA, [0x77] = RRA,
    [0x87] = SAX, [0x97] = SAX, [0xA7] = LAX, [0xB7] = LAX,
    [0xC7] = DCP, [0xD7] = DCP, [0xE7] = ISC, [0xF7] = ISC,

    // xB column: undocumented on NMOS (NOP/WAI/STP on CMOS)
    [0x0B] = ANC, [0x1B] = SLO, [0x2B] = ANC, [0x3B] = RLA,
    [0x4B] = ALR, [0x5B] = SRE, [0x6B] = ARR, [0x7B] = RRA,
    [0x8B] = op_illegal, [0x9B] = op_illegal, [0xAB] = op_illegal,
    [0xBB] = op_illegal, [0xCB] = op_illegal, [0xDB] = op_illegal,
    [0xEB] = ALT_SBC, [0xFB] = ISC,

    // xF column: undocumented on NMOS (BBR/BBS on CMOS)
    [0x0F] = SLO, [0x1F] = SLO, [0x2F] = RLA, [0x3F] = RLA,
    [0x4F] = SRE, [0x5F] = SRE, [0x6F] = RRA, [0x7F] = RRA,
    [0x8F] = SAX, [0x9F] = op_illegal, [0xAF] = LAX, [0xBF] = LAX,
    [0xCF] = DCP, [0xDF] = DCP, [0xEF] = ISC, [0xFF] = ISC,

    // Other differences
    [0x1A] = NOP,        // NMOS: NOP, CMOS: INC A
    [0x3A] = NOP,        // NMOS: NOP, CMOS: DEC A
    [0x5A] = NOP,        // NMOS: NOP, CMOS: PHY
    [0x7A] = NOP,        // NMOS: NOP, CMOS: PLY
    [0x7C] = NOP_READ,   // NMOS: NOP abs,X, CMOS: JMP (abs,X)
    [0x89] = NOP,        // NMOS: NOP #imm, CMOS: BIT #imm
    [0xDA] = NOP,        // NMOS: NOP, CMOS: PHX
    [0xFA] = NOP,        // NMOS: NOP, CMOS: PLX
};

// ---------------------------------------------------------------------------
// Variant-aware dispatch — called from cpu_step
// ---------------------------------------------------------------------------
void instr_dispatch(void) {
    if (cpu_get_variant() == CPU_VARIANT_NMOS_6502) {
        instr_fn h = NMOS_HANDLER[OPCODE];
        if (h) { h(); return; }
    }
    INSTR_HANDLERS[OPCODE]();
}