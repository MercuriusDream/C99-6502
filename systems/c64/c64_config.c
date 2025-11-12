#include "system_config.h"
#include "memory.h"
#include <stdio.h>

static MEM_WORD c64_vic_read(MEM_TWO_WORDS addr, void* ctx) {
    (void)ctx;
    (void)addr;
    return 0x00;
}

static void c64_vic_write(MEM_TWO_WORDS addr, MEM_WORD data, void* ctx) {
    (void)ctx;
    printf("[VIC-II] $%04X = $%02X\n", addr, data);
}

static MEM_WORD c64_sid_read(MEM_TWO_WORDS addr, void* ctx) {
    (void)ctx;
    (void)addr;
    return 0x00;
}

static void c64_sid_write(MEM_TWO_WORDS addr, MEM_WORD data, void* ctx) {
    (void)ctx;
    printf("[SID] $%04X = $%02X\n", addr, data);
}

static void c64_init_peripherals(void) {
    printf("Initializing C64 peripherals...\n");
    mem_region_add_io(0xD000, 0x400, c64_vic_read, c64_vic_write, NULL);
    mem_region_add_io(0xD400, 0x400, c64_sid_read, c64_sid_write, NULL);
    printf("  VIC-II: $D000-$D3FF\n");
    printf("  SID: $D400-$D7FF\n");
}

static const SYSTEM_CONFIG c64_config = {
    .name = "Commodore 64",
    .cpu_variant = CPU_VARIANT_NMOS_6502,
    .ram_start = 0x0000,
    .ram_size = 0xC000,
    .rom_start = 0xA000,
    .rom_size = 8 * 1024,
    .reset_vector = 0xA000,
    .init_peripherals = c64_init_peripherals,
    .post_init = NULL
};

const SYSTEM_CONFIG* c64_get_config(void) {
    return &c64_config;
}
