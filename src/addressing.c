#include "addressing.h"
#include "memory.h"
#include "bus.h"
#include "cpu.h"

MEM_TWO_WORDS addr_zp() {
    return (MEM_TWO_WORDS)fetch8();
}

MEM_TWO_WORDS addr_zpx() {
    return (MEM_TWO_WORDS)((REG.X + fetch8()) & 0xFF);
}

MEM_TWO_WORDS addr_zpy() {
    return (MEM_TWO_WORDS)((REG.Y + fetch8()) & 0xFF);
}

MEM_TWO_WORDS addr_abs() {
    return fetch16();
}

MEM_TWO_WORDS addr_absx(int* page_cross) {
    MEM_TWO_WORDS base = fetch16();
    MEM_TWO_WORDS addr = base + (MEM_TWO_WORDS)REG.X;
    *page_cross = ((base&0xFF00) != (addr&0xFF00)) ? 1:0;
    return addr;
}

MEM_TWO_WORDS addr_absy(int* page_cross) {
    MEM_TWO_WORDS base = fetch16();
    MEM_TWO_WORDS addr = base + (MEM_TWO_WORDS)REG.Y;
    *page_cross = ((base&0xFF00) != (addr&0xFF00)) ? 1:0;
    return addr;
}

MEM_TWO_WORDS addr_ind() {
    MEM_TWO_WORDS base = fetch16();
    // NMOS 6502 JMP indirect bug: if address is $xxFF, wraps within page
    // CMOS 65C02 fixes this bug - reads correctly across page boundary
    if (cpu_get_variant() == CPU_VARIANT_NMOS_6502 && (base & 0xFF) == 0xFF) {
        MEM_WORD lo = bus_read(base);
        MEM_WORD hi = bus_read(base & 0xFF00);  // Bug: wraps within page
        return (MEM_TWO_WORDS)(lo | ((MEM_TWO_WORDS)hi << 8));
    }
    return bus_read16(base);  // Normal read (65C02 always uses this)
}

MEM_TWO_WORDS addr_indx() {
    MEM_WORD zp_addr = (fetch8() + REG.X) & 0xFF;
    MEM_WORD lo = bus_read(zp_addr);
    MEM_WORD hi = bus_read((zp_addr + 1) & 0xFF);
    return (MEM_TWO_WORDS)(lo | ((MEM_TWO_WORDS)hi << 8));
}

MEM_TWO_WORDS addr_indy(int* page_cross) {
    MEM_WORD zp_addr = fetch8();
    MEM_WORD lo = bus_read(zp_addr);
    MEM_WORD hi = bus_read((zp_addr + 1) & 0xFF);
    MEM_TWO_WORDS base = (MEM_TWO_WORDS)(lo | ((MEM_TWO_WORDS)hi << 8));
    MEM_TWO_WORDS addr = base + (MEM_TWO_WORDS)REG.Y;
    *page_cross = ((base&0xFF00) != (addr&0xFF00)) ? 1:0;
    return addr;
}

MEM_TWO_WORDS addr_imm() {
    return (MEM_TWO_WORDS)fetch8();
}

MEM_TWO_WORDS addr_rel() {
    // Must fetch offset first, then use updated PC
    // Doing "REG.PC + fetch8()" is undefined behavior since fetch8() modifies REG.PC
    SIGNED_MEM_WORD offset = (SIGNED_MEM_WORD)fetch8();
    return (MEM_TWO_WORDS)REG.PC + offset;
}

MEM_TWO_WORDS addr_zprel() {
    MEM_WORD zp_addr = fetch8();
    REL_OFFSET = (SIGNED_MEM_WORD)fetch8();
    return (MEM_TWO_WORDS)zp_addr;
}

MEM_TWO_WORDS addr_zp_ind() {
    MEM_WORD zp_addr = fetch8();
    MEM_WORD lo = bus_read(zp_addr);
    MEM_WORD hi = bus_read((zp_addr + 1) & 0xFF);
    return (MEM_TWO_WORDS)(lo | ((MEM_TWO_WORDS)hi << 8));
}

MEM_TWO_WORDS addr_abs_ind_x() {
    MEM_TWO_WORDS abs_addr = fetch16();
    MEM_TWO_WORDS indexed_addr = abs_addr + (MEM_TWO_WORDS)REG.X;
    return bus_read16(indexed_addr);
}

MEM_TWO_WORDS addr_resolve(ADDR_MODE mode, int* page_cross, int* has_ea) {
    if (page_cross) *page_cross = 0;
    if (has_ea) *has_ea = 1;

    switch (mode) {
        case ADDR_ZP:   return addr_zp();
        case ADDR_ZPX:  return addr_zpx();
        case ADDR_ZPY:  return addr_zpy();
        case ADDR_ABS:  return addr_abs();
        case ADDR_ABSX: return addr_absx(page_cross);
        case ADDR_ABSY: return addr_absy(page_cross);
        case ADDR_IND:  return addr_ind();
        case ADDR_INDX: return addr_indx();
        case ADDR_INDY: return addr_indy(page_cross);
        case ADDR_ZP_IND: return addr_zp_ind();
        case ADDR_ABS_IND_X: return addr_abs_ind_x();
        case ADDR_IMM:
            if (has_ea) *has_ea = 0;
            return addr_imm();
        case ADDR_REL:
            if (has_ea) *has_ea = 0;
            return addr_rel();
        case ADDR_ZPREL:
            return addr_zprel();
        case ADDR_ACC:
            if (has_ea) *has_ea = 0;
            return 0;
        case ADDR_NONE:
        default:
            if (has_ea) *has_ea = 0;
            return 0;
    }
}