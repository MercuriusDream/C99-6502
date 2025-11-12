#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "instruments_table.h"
#include "instruments_implementation.h"
#include "memory.h"
#include "trace.h"
#include "loader.h"
#include "system_config.h"
#include "display.h"

typedef enum {
    ARG_UNKNOWN = 0,
    ARG_TRACE,
    ARG_FILE,
    ARG_ADDRESS,
    ARG_CPU,
    ARG_RAM_START,
    ARG_RAM_SIZE,
    ARG_ROM_START,
    ARG_ROM_SIZE,
    ARG_SYSTEM
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
    } else if (strcmp(arg, "--system") == 0) {
        return ARG_SYSTEM;
    }
    return ARG_UNKNOWN;
}

int main(int argc, char** argv) {
    int enable_trace = 0;
    const char* rom_file = NULL;
    MEM_TWO_WORDS rom_addr = 0x8000;
    CPU_VARIANT cpu_variant = CPU_VARIANT_NMOS_6502;
    const SYSTEM_CONFIG* sys_config = NULL;
    int cpu_variant_set = 0;
    int rom_addr_set = 0;
    int ram_start_set = 0;
    int ram_size_set = 0;
    int rom_start_set = 0;
    int rom_size_set = 0;

    MEM_TWO_WORDS ram_start = 0x0000;
    MEM_TWO_WORDS ram_size = 0x8000;
    MEM_TWO_WORDS rom_start = 0x8000;
    MEM_TWO_WORDS rom_size = 0x8000;

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
                    rom_addr_set = 1;
                }
                break;

            case ARG_RAM_START:
                if (i + 1 < argc) {
                    ram_start = (MEM_TWO_WORDS)strtol(argv[++i], NULL, 16);
                    ram_start_set = 1;
                }
                break;

            case ARG_RAM_SIZE:
                if (i + 1 < argc) {
                    ram_size = (MEM_TWO_WORDS)strtol(argv[++i], NULL, 0);
                    ram_size_set = 1;
                }
                break;

            case ARG_ROM_START:
                if (i + 1 < argc) {
                    rom_start = (MEM_TWO_WORDS)strtol(argv[++i], NULL, 16);
                    rom_start_set = 1;
                }
                break;

            case ARG_ROM_SIZE:
                if (i + 1 < argc) {
                    rom_size = (MEM_TWO_WORDS)strtol(argv[++i], NULL, 0);
                    rom_size_set = 1;
                }
                break;

            case ARG_CPU:
                if (i + 1 < argc) {
                    i++;
                    if (strcmp(argv[i], "6502") == 0 || strcmp(argv[i], "nmos") == 0) {
                        cpu_variant = CPU_VARIANT_NMOS_6502;
                        cpu_variant_set = 1;
                    } else if (strcmp(argv[i], "65c02") == 0 || strcmp(argv[i], "cmos") == 0) {
                        cpu_variant = CPU_VARIANT_CMOS_65C02;
                        cpu_variant_set = 1;
                    } else {
                        printf("Unknown CPU variant: %s\n", argv[i]);
                        printf("Valid options: 6502, nmos, 65c02, cmos\n");
                        return 1;
                    }
                }
                break;

            case ARG_SYSTEM:
                if (i + 1 < argc) {
                    sys_config = system_get_config(argv[++i]);
                    if (!sys_config) {
                        printf("Unknown system: %s\n", argv[i]);
                        system_list_configs();
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

    if (sys_config) {
        printf("C99-6502 [%s Mode]\n", sys_config->name);
        if (!cpu_variant_set) {
            cpu_variant = sys_config->cpu_variant;
        }
        if (!ram_start_set) {
            ram_start = sys_config->ram_start;
        }
        if (!ram_size_set) {
            ram_size = sys_config->ram_size;
        }
        if (!rom_start_set) {
            rom_start = sys_config->rom_start;
        }
        if (!rom_size_set) {
            rom_size = sys_config->rom_size;
        }
        if (!rom_addr_set) {
            rom_addr = sys_config->reset_vector;
        }
    } else {
        printf("C99-6502\n");
    }

    cpu_set_variant(cpu_variant);

    printf("CPU Variant: %s\n",
           cpu_variant == CPU_VARIANT_CMOS_65C02 ? "CMOS 65C02" : "NMOS 6502");

    mem_region_clear();

    if (mem_region_add_ram(ram_start, ram_size) != 0) {
        printf("Error: Failed to allocate RAM region\n");
        return 1;
    }

    if (mem_region_add_rom(rom_start, rom_size) != 0) {
        printf("Error: Failed to allocate ROM region\n");
        return 1;
    }

    if (sys_config && sys_config->init_peripherals) {
        sys_config->init_peripherals();
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
        mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, rom_addr); // Setting the Memory Vector
        printf("Reset the vector to $%04X\n", rom_addr);
    } else if (!sys_config) {
        // Only load sample ROM if not using a system config
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
        mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, rom_addr); // Setting the Memory Vector
        printf("Reset the vector to $%04X\n", rom_addr);
    } else {
        // System config will load its own ROM in init_peripherals
        printf("System ROM will be loaded by system config...\n");
    }

    mem_region_init(); // Initialize the Memory region
    printf("Memory bus connected\n");

    // Set reset vector if using system config and no ROM file was loaded
    if (sys_config && !rom_file) {
        mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, sys_config->reset_vector);
        printf("Reset vector set to $%04X\n", sys_config->reset_vector);
    }

    // Call post-init if system config has one (e.g., clear screen)
    if (sys_config && sys_config->post_init) {
        sys_config->post_init();
    }

    cpu_reset(); // Reset the CPU state

    // Set up registers for Disk II ROM boot (slot 6)
    // The ROM expects X = slot * $10
    if (sys_config && strcmp(sys_config->name, "Apple II") == 0) {
        REG.X = 0x60;  // Slot 6
        printf("Set X=$60 for Disk II ROM (slot 6)\n");
    }

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
    printf("[DEBUG] PC before run loop: $%04X, SP=$%02X\n", REG.PC, REG.S);

    // Check if we're running an interactive system (Apple II with display)
    int is_interactive = (sys_config != NULL && strcmp(sys_config->name, "Apple II") == 0);

    if (is_interactive) {
        // Interactive mode with SDL display
        printf("Interactive mode enabled. Press ESC to quit.\n");

        const int CYCLES_PER_FRAME = 17030;  // ~60 Hz at 1 MHz
        const int NSEC_PER_CYCLE = 1000;     // 1 MHz = 1000 ns per cycle

        struct timespec start, now;
        clock_gettime(CLOCK_MONOTONIC, &start);
        long long total_cycles = 0;

        // Debug: Track PC to detect loops
        static MEM_TWO_WORDS last_pc = 0xFFFF;
        static int repeat_count = 0;
        static long long last_debug_cycle = 0;

        // Debug: Monitor boot sector area for writes
        static MEM_WORD boot_sector_0800 = 0xFF;
        static MEM_WORD boot_sector_0900 = 0xFF;
        static int boot_load_reported = 0;

        // Debug: Monitor zero page for disk decode activity
        static MEM_WORD zp_26 = 0xFF;  // ROM uses $26-$2B for decode buffer
        static int zp_activity_reported = 0;

        int quit = 0;
        while (!quit) {
            // Handle SDL events (keyboard, quit)
            quit = display_handle_events();

            // Run one frame worth of cycles
            for (int i = 0; i < CYCLES_PER_FRAME && !quit; i++) {
                // Track when PC leaves Disk II ROM area
                static int in_disk_rom = 0;
                static int disk_rom_exit_logged = 0;

                if (REG.PC >= 0xC600 && REG.PC <= 0xC6FF) {
                    in_disk_rom = 1;
                } else if (in_disk_rom && !disk_rom_exit_logged) {
                    printf("[ROM EXIT] Disk II ROM exited at cycle %lld\n", total_cycles);
                    printf("  Last PC in ROM: $C6xx, jumped to: $%04X\n", REG.PC);
                    printf("  A=$%02X, X=$%02X, Y=$%02X, SP=$%02X, P=$%02X\n",
                           REG.A, REG.X, REG.Y, REG.S, REG.P);
                    printf("  Zero page $26-$2D: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                           bus_read(0x26), bus_read(0x27), bus_read(0x28), bus_read(0x29),
                           bus_read(0x2A), bus_read(0x2B), bus_read(0x2C), bus_read(0x2D));
                    disk_rom_exit_logged = 1;
                    in_disk_rom = 0;
                }

                cpu_step();
                total_cycles++;

                // Check zero page for decode activity
                MEM_WORD zp_byte = bus_read(0x26);
                if (zp_byte != zp_26 && !zp_activity_reported && total_cycles > 1000) {
                    printf("[ZP DETECT] Byte at $26 changed: $%02X -> $%02X (cycle %lld, PC=$%04X)\n",
                           zp_26, zp_byte, total_cycles, REG.PC);
                    zp_26 = zp_byte;
                    if (zp_byte != 0x00 && zp_byte != 0xFF) {
                        zp_activity_reported = 1;
                        printf("[ZP DETECT] Disk decode activity detected!\n");
                        printf("  $26-$2D: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                               bus_read(0x26), bus_read(0x27), bus_read(0x28), bus_read(0x29),
                               bus_read(0x2A), bus_read(0x2B), bus_read(0x2C), bus_read(0x2D));
                    }
                }

                // Check if boot sector is being loaded
                MEM_WORD boot_byte = bus_read(0x0800);
                MEM_WORD boot_byte_0900 = bus_read(0x0900);
                if (boot_byte != boot_sector_0800 && !boot_load_reported) {
                    printf("[BOOT DETECT] Byte at $0800 changed: $%02X -> $%02X (cycle %lld, PC=$%04X)\n",
                           boot_sector_0800, boot_byte, total_cycles, REG.PC);
                    boot_sector_0800 = boot_byte;
                    if (boot_byte == 0x01) {  // First byte of ProDOS boot sector
                        printf("[BOOT DETECT] Boot sector loading detected!\n");
                        printf("  $0800-$0807: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                               bus_read(0x0800), bus_read(0x0801), bus_read(0x0802), bus_read(0x0803),
                               bus_read(0x0804), bus_read(0x0805), bus_read(0x0806), bus_read(0x0807));
                    }
                }
                if (boot_byte_0900 != boot_sector_0900 && !boot_load_reported) {
                    printf("[BOOT DETECT] Byte at $0900 changed: $%02X -> $%02X (cycle %lld, PC=$%04X)\n",
                           boot_sector_0900, boot_byte_0900, total_cycles, REG.PC);
                    boot_sector_0900 = boot_byte_0900;
                    if (boot_byte_0900 == 0x01) {
                        boot_load_reported = 1;
                        printf("[BOOT DETECT] Second boot sector detected!\n");
                        printf("  $0900-$0907: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                               bus_read(0x0900), bus_read(0x0901), bus_read(0x0902), bus_read(0x0903),
                               bus_read(0x0904), bus_read(0x0905), bus_read(0x0906), bus_read(0x0907));
                    }
                }

                // Debug output every 100000 cycles
                if (total_cycles - last_debug_cycle >= 100000) {
                    printf("[DEBUG] Cycle %lld, PC=$%04X, A=$%02X, X=$%02X, Y=$%02X, SP=$%02X\n",
                           total_cycles, REG.PC, REG.A, REG.X, REG.Y, REG.S);
                    // Show first few bytes of screen memory
                    printf("  Screen: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                           bus_read(0x0400), bus_read(0x0401), bus_read(0x0402), bus_read(0x0403),
                           bus_read(0x0404), bus_read(0x0405), bus_read(0x0406), bus_read(0x0407));
                    // Show boot sector area
                    printf("  Boot: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                           bus_read(0x0800), bus_read(0x0801), bus_read(0x0802), bus_read(0x0803),
                           bus_read(0x0804), bus_read(0x0805), bus_read(0x0806), bus_read(0x0807));
                    last_debug_cycle = total_cycles;
                }

                // Check if PC hasn't changed in a suspicious way
                if (REG.PC == last_pc) {
                    repeat_count++;
                    if (repeat_count > 1000000) {
                        printf("[ERROR] CPU stuck in infinite loop at PC=$%04X\n", REG.PC);
                        printf("  A=$%02X, X=$%02X, Y=$%02X, SP=$%02X, P=$%02X\n",
                               REG.A, REG.X, REG.Y, REG.S, REG.P);
                        printf("  Memory at PC: %02X %02X %02X %02X\n",
                               bus_read(REG.PC), bus_read(REG.PC+1),
                               bus_read(REG.PC+2), bus_read(REG.PC+3));
                        quit = 1;
                        break;
                    }
                } else {
                    last_pc = REG.PC;
                    repeat_count = 0;
                }

                // Check for BRK if not in Apple II mode
                if (!(REG.PC) && !is_interactive) {
                    printf("Found BRK at Cycle %lld, Execution terminated.\n", total_cycles);
                    quit = 1;
                    break;
                }
            }

            // Refresh display
            display_refresh();

            // Throttle to ~1 MHz
            clock_gettime(CLOCK_MONOTONIC, &now);
            long long elapsed_nsec = (now.tv_sec - start.tv_sec) * 1000000000LL +
                                      (now.tv_nsec - start.tv_nsec);
            long long target_nsec = total_cycles * NSEC_PER_CYCLE;

            if (target_nsec > elapsed_nsec) {
                struct timespec sleep_time = {
                    .tv_sec = 0,
                    .tv_nsec = target_nsec - elapsed_nsec
                };
                nanosleep(&sleep_time, NULL);
            }
        }

        display_cleanup();
        printf("\nEmulation stopped.\n");
    } else {
        // Non-interactive mode (original behavior)
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
    }

    return 0;
}
