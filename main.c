#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "instructions_table.h"
#include "instructions_implementation.h"
#include "memory.h"
#include "trace.h"
#include "loader.h"

typedef enum {
    ARG_UNKNOWN = 0,
    ARG_TRACE,
    ARG_FILE,
    ARG_ADDRESS,
    ARG_CPU,
    ARG_RAM_START,
    ARG_RAM_SIZE,
    ARG_ROM_START,
    ARG_ROM_SIZE
} ArgType;

static ArgType parse_arg(const char* arg) {
    if (strcmp(arg, "-t") == 0 || strcmp(arg, "--trace") == 0) {
        return ARG_TRACE;
    } else if (strcmp(arg, "-f") == 0 || strcmp(arg, "--file") == 0) {
        return ARG_FILE;
    } else if (strcmp(arg, "-a") == 0 || strcmp(arg, "--address") == 0) {
        return ARG_ADDRESS;
    } else if (strcmp(arg, "-c") == 0 || strcmp(arg, "--cpu") == 0) {
        return ARG_CPU;
    } else if (strcmp(arg, "-r") == 0 || strcmp(arg, "--ram-start") == 0) {
        return ARG_RAM_START;
    } else if (strcmp(arg, "-R") == 0 || strcmp(arg, "--ram-size") == 0) {
        return ARG_RAM_SIZE;
    } else if (strcmp(arg, "-s") == 0 || strcmp(arg, "--rom-start") == 0) {
        return ARG_ROM_START;
    } else if (strcmp(arg, "-S") == 0 || strcmp(arg, "--rom-size") == 0) {
        return ARG_ROM_SIZE;
    }
    return ARG_UNKNOWN;
}

int main(int argc, char** argv) {
    int enable_trace = 0;
    const char* rom_file = NULL;
    MEM_TWO_WORDS rom_addr = 0x8000;
    CPU_VARIANT cpu_variant = CPU_VARIANT_NMOS_6502;  // Default to NMOS

    // Memory configuration defaults
    MEM_TWO_WORDS ram_start = 0x0000;
    MEM_TWO_WORDS ram_size = 0x8000;  // 32KB
    MEM_TWO_WORDS rom_start = 0x8000;
    MEM_TWO_WORDS rom_size = 0x8000;  // 32KB

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        ArgType arg_type = parse_arg(argv[i]);

        switch (arg_type) {
            case ARG_TRACE:
                enable_trace = 1;
                break;

            case ARG_FILE:
                if (i + 1 < argc) {
                    rom_file = argv[++i];
                }
                break;

            case ARG_ADDRESS:
                if (i + 1 < argc) {
                    rom_addr = (MEM_TWO_WORDS)strtol(argv[++i], NULL, 16);
                }
                break;

            case ARG_RAM_START:
                if (i + 1 < argc) {
                    ram_start = (MEM_TWO_WORDS)strtol(argv[++i], NULL, 16);
                }
                break;

            case ARG_RAM_SIZE:
                if (i + 1 < argc) {
                    ram_size = (MEM_TWO_WORDS)strtol(argv[++i], NULL, 0);
                }
                break;

            case ARG_ROM_START:
                if (i + 1 < argc) {
                    rom_start = (MEM_TWO_WORDS)strtol(argv[++i], NULL, 16);
                }
                break;

            case ARG_ROM_SIZE:
                if (i + 1 < argc) {
                    rom_size = (MEM_TWO_WORDS)strtol(argv[++i], NULL, 0);
                }
                break;

            case ARG_CPU:
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
                break;

            case ARG_UNKNOWN:
            default:
                printf("Unknown argument: %s\n", argv[i]);
                break;
        }
    }

    // Set CPU variant
    cpu_set_variant(cpu_variant);

    printf("C99-6502\n");
    printf("CPU Variant: %s\n",
           cpu_variant == CPU_VARIANT_CMOS_65C02 ? "CMOS 65C02" : "NMOS 6502"); // CPU Variant

    mem_region_clear(); // Memory Cleanup

    if (mem_region_add_ram(ram_start, ram_size) != 0) {
        printf("Error: Failed to allocate RAM region\n");
        return 1;
    }

    if (mem_region_add_rom(rom_start, rom_size) != 0) {
        printf("Error: Failed to allocate ROM region\n");
        return 1;
    }

    printf("Memory map configuration:\n");
    printf("  $%04X-$%04X: RAM (%uKB)\n", ram_start, ram_start + ram_size - 1, ram_size / 1024);
    printf("  $%04X-$%04X: ROM (%uKB)\n\n", rom_start, rom_start + rom_size - 1, rom_size / 1024);

    if (rom_file) { // Loading the ROM file
        if (load_bin_region(rom_file, rom_addr) == 0) {
            printf("ROM loaded from: %s at $%04X.\n", rom_file, rom_addr);
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

    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, rom_addr); // Setting the Memory Vector
    printf("Reset the vector to $%04X\n", rom_addr);

    mem_region_init(); // Initialize the Memory region
    printf("Memory bus connected\n");

    cpu_reset(); // Reset the CPU state
    printf("CPU reset has been completed:\n");
    printf("  PC: $%04X\n", REG.PC);
    printf("  SP: $%02X\n", REG.S);
    printf("  P:  $%02X\n", REG.P);
    printf("  A:  $%02X, X: $%02X, Y: $%02X\n", REG.A, REG.X, REG.Y);

    if (enable_trace) { // Enable Trace if needed
        trace_set_enabled(1);
        printf("Trace : Enabled\n");
    }

    printf("Running...\n");
    for (int cycle_cnt=1; cycle_cnt<=CPU_TEST_RUN_LIMIT; cycle_cnt++) {
        cpu_step();
        if (!(REG.PC)) {
            printf("Found BRK at Cycle %d, Execution terminated.\n", cycle_cnt);
            break;
        }
    }

    printf("\nExecution complete.\n");
    printf("  PC: $%04X\n", REG.PC);
    printf("  A:  $%02X, X: $%02X, Y: $%02X\n", REG.A, REG.X, REG.Y);
    printf("  Memory at $0200: $%02X\n", bus_read(0x0200));

    return 0;
}
