#include "system_config.h"
#include "memory.h"
#include <stdio.h>

static MEM_WORD nes_ppu_read(MEM_TWO_WORDS addr, void* ctx) {
    (void)ctx;
    (void)addr;
    return 0x00;
}

static void nes_ppu_write(MEM_TWO_WORDS addr, MEM_WORD data, void* ctx) {
    (void)ctx;

    switch (addr) {
        case 0x2000:
            printf("[PPU] PPUCTRL = $%02X\n", data);
            break;
        case 0x2001:
            printf("[PPU] PPUMASK = $%02X\n", data);
            break;
        case 0x2003:
            printf("[PPU] OAMADDR = $%02X\n", data);
            break;
        case 0x2004:
            printf("[PPU] OAMDATA = $%02X\n", data);
            break;
        case 0x2005:
            printf("[PPU] PPUSCROLL = $%02X\n", data);
            break;
        case 0x2006:
            printf("[PPU] PPUADDR = $%02X\n", data);
            break;
        case 0x2007:
            printf("[PPU] PPUDATA = $%02X\n", data);
            break;
    }
}

static MEM_WORD nes_apu_read(MEM_TWO_WORDS addr, void* ctx) {
    (void)ctx;
    (void)addr;
    return 0x00;
}

static void nes_apu_write(MEM_TWO_WORDS addr, MEM_WORD data, void* ctx) {
    (void)ctx;
    printf("[APU] $%04X = $%02X\n", addr, data);
}

static void nes_init_peripherals(void) {
    printf("Initializing NES peripherals...\n");
    mem_region_add_io(0x2000, 0x8, nes_ppu_read, nes_ppu_write, NULL);
    mem_region_add_io(0x4000, 0x18, nes_apu_read, nes_apu_write, NULL);
    printf("  PPU registers: $2000-$2007\n");
    printf("  APU/IO registers: $4000-$4017\n");
}

static const SYSTEM_CONFIG nes_config = {
    .name = "NES",
    .cpu_variant = CPU_VARIANT_NMOS_6502,
    .ram_start = 0x0000,
    .ram_size = 2 * 1024,
    .rom_start = 0x8000,
    .rom_size = 32 * 1024,
    .reset_vector = 0x8000,
    .init_peripherals = nes_init_peripherals,
    .post_init = NULL
};

const SYSTEM_CONFIG* nes_get_config(void) {
    return &nes_config;
}
