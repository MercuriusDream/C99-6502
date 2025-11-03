#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "instruments_table.h"
#include "instruments_implementation.h"
#include "memory.h"
#include "trace.h"
#include "loader.h"

int main(int argc, char** argv) {
    int enable_trace = 0;
    const char* rom_file = NULL;
    MEM_TWO_WORDS rom_addr = 0x8000;
    CPU_VARIANT cpu_variant = CPU_VARIANT_NMOS_6502;  // Default to NMOS

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0) {
            enable_trace = 1;
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            rom_file = argv[++i];
        } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            rom_addr = (MEM_TWO_WORDS)strtol(argv[++i], NULL, 16);
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cpu") == 0) {
            if (i + 1 < argc) {
                i++;
                if (strcmp(argv[i], "6502") == 0 || strcmp(argv[i], "nmos") == 0) {
                    cpu_variant = CPU_VARIANT_NMOS_6502;
                } else if (strcmp(argv[i], "65c02") == 0 || strcmp(argv[i], "cmos") == 0) {
                    cpu_variant = CPU_VARIANT_CMOS_65C02;
                } else {
                    printf("Unknown CPU variant: %s\n", argv[i]);
                    printf("Valid options: 6502, nmos, 65c02, cmos\n");
                    return 1;
                }
            }
        }
    }

    // Set CPU variant
    cpu_set_variant(cpu_variant);

    printf("6502 Emulator Starting...\n");
    printf("CPU Variant: %s\n",
           cpu_variant == CPU_VARIANT_CMOS_65C02 ? "CMOS 65C02" : "NMOS 6502");
    printf("Using region-based memory system...\n\n");

    // 1. Setup memory regions
    // 32KB RAM: $0000-$7FFF (typical for many 6502 systems)
    // 32KB ROM: $8000-$FFFF (program code, vectors)
    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);  // 32KB RAM
    mem_region_add_rom(0x8000, 0x8000);  // 32KB ROM
    printf("Memory map configured:\n");
    printf("  $0000-$7FFF: RAM (32KB)\n");
    printf("  $8000-$FFFF: ROM (32KB)\n\n");

    // 2. Load ROM
    if (rom_file) {
        if (load_bin_region(rom_file, rom_addr) == 0) {
            printf("ROM loaded from %s at $%04X.\n", rom_file, rom_addr);
        } else {
            printf("Failed to load ROM from %s\n", rom_file);
            return 1;
        }
    } else {
        // Load sample ROM: LDA #$42, STA $0200, INX, INY, BRK
        MEM_WORD sample_rom[] = {
            0xA9, 0x42,        // LDA #$42
            0x8D, 0x00, 0x02,  // STA $0200
            0xE8,              // INX
            0xC8,              // INY
            0x00               // BRK
        };
        mem_region_load(rom_addr, sample_rom, sizeof(sample_rom));
        printf("Sample ROM loaded at $%04X.\n", rom_addr);
    }

    // 3. Set reset vector
    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, rom_addr);
    printf("Reset vector set to $%04X.\n", rom_addr);

    // 4. Connect bus
    mem_region_init();
    printf("Bus connected.\n");

    // 5. CPU reset
    cpu_reset();
    printf("CPU reset complete.\n");
    printf("  PC: $%04X\n", REG.PC);
    printf("  SP: $%02X\n", REG.S);
    printf("  P:  $%02X\n", REG.P);
    printf("  A:  $%02X, X: $%02X, Y: $%02X\n\n", REG.A, REG.X, REG.Y);

    // 6. Enable trace if requested
    if (enable_trace) {
        trace_set_enabled(1);
        printf("Trace enabled.\n\n");
    }

    // 7. Run
    printf("Running...\n");
    if (enable_trace) {
        // Run step by step with trace
        for (int i = 0; i < 20; i++) {
            cpu_step();
            if (REG.PC == 0) break;  // Stop on BRK vector not set
        }
    } else {
        cpu_run(100);
    }

    printf("\nExecution complete.\n");
    printf("  PC: $%04X\n", REG.PC);
    printf("  A:  $%02X, X: $%02X, Y: $%02X\n", REG.A, REG.X, REG.Y);
    printf("  Memory at $0200: $%02X\n", bus_read(0x0200));

    return 0;
}
