#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "loader.h"
#include "debugger.h"
#include "interactive_debugger.h"
#include "trace.h"
#include "system_config.h"

static void print_usage(const char* prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -f, --file <file>      Binary file to load\n");
    printf("  -a, --address <addr>   Load address in hex (e.g., 8000)\n");
    printf("  -c, --cpu <variant>    CPU variant: 6502, nmos, 65c02, cmos (default: nmos)\n");
    printf("  --system <name>        Use system config: apple2, nes, c64\n");
    printf("  -r, --ram-start <addr> RAM start address in hex (default: 0000)\n");
    printf("  -R, --ram-size <size>  RAM size in bytes (default: 32768)\n");
    printf("  -s, --rom-start <addr> ROM start address in hex (default: 8000)\n");
    printf("  -S, --rom-size <size>  ROM size in bytes (default: 32768)\n");
    printf("  -h, --help             Show this help\n");
}

int main(int argc, char** argv) {
    char* bin_file = NULL;
    MEM_TWO_WORDS load_addr = 0x8000;
    CPU_VARIANT variant = CPU_VARIANT_NMOS_6502;
    const SYSTEM_CONFIG* sys_config = NULL;
    MEM_TWO_WORDS ram_start = 0x0000;
    MEM_TWO_WORDS ram_size = 32768;
    MEM_TWO_WORDS rom_start = 0x8000;
    MEM_TWO_WORDS rom_size = 32768;

    static struct option long_options[] = {
        {"file",      required_argument, 0, 'f'},
        {"address",   required_argument, 0, 'a'},
        {"cpu",       required_argument, 0, 'c'},
        {"system",    required_argument, 0, 'y'},
        {"ram-start", required_argument, 0, 'r'},
        {"ram-size",  required_argument, 0, 'R'},
        {"rom-start", required_argument, 0, 's'},
        {"rom-size",  required_argument, 0, 'S'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:a:c:y:r:R:s:S:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'f':
                bin_file = optarg;
                break;
            case 'a':
                load_addr = (MEM_TWO_WORDS)strtol(optarg, NULL, 16);
                break;
            case 'c':
                if (strcmp(optarg, "65c02") == 0 || strcmp(optarg, "cmos") == 0) {
                    variant = CPU_VARIANT_CMOS_65C02;
                } else {
                    variant = CPU_VARIANT_NMOS_6502;
                }
                break;
            case 'y':
                sys_config = system_get_config(optarg);
                if (!sys_config) {
                    printf("Unknown system: %s\n", optarg);
                    system_list_configs();
                    return 1;
                }
                break;
            case 'r':
                ram_start = (MEM_TWO_WORDS)strtol(optarg, NULL, 16);
                break;
            case 'R':
                ram_size = (MEM_TWO_WORDS)atoi(optarg);
                break;
            case 's':
                rom_start = (MEM_TWO_WORDS)strtol(optarg, NULL, 16);
                break;
            case 'S':
                rom_size = (MEM_TWO_WORDS)atoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (sys_config) {
        printf("=== MOS 6502 Interactive Debugger [%s Mode] ===\n", sys_config->name);
        variant = sys_config->cpu_variant;
        ram_start = sys_config->ram_start;
        ram_size = sys_config->ram_size;
        rom_start = sys_config->rom_start;
        rom_size = sys_config->rom_size;
        load_addr = sys_config->reset_vector;
    } else {
        printf("=== MOS 6502 Interactive Debugger ===\n");
    }

    printf("CPU Variant: %s\n", variant == CPU_VARIANT_CMOS_65C02 ? "65C02 (CMOS)" : "6502 (NMOS)");
    printf("Memory: RAM $%04X-$%04X, ROM $%04X-$%04X\n\n",
           ram_start, ram_start + ram_size - 1,
           rom_start, rom_start + rom_size - 1);

    mem_region_clear();
    mem_region_add_ram(ram_start, ram_size);
    mem_region_add_rom(rom_start, rom_size);

    if (sys_config && sys_config->init_peripherals) {
        sys_config->init_peripherals();
    }

    if (bin_file) {
        printf("Loading %s at $%04X...\n", bin_file, load_addr);
        if (load_bin_region(bin_file, load_addr) != 0) {
            fprintf(stderr, "Error: Failed to load binary file\n");
            return 1;
        }
    }

    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, load_addr);
    mem_region_init();

    cpu_set_variant(variant);
    cpu_init();
    debugger_init();
    idbg_init();

    cpu_reset();

    idbg_start();

    while (idbg_get_mode() != IDBG_MODE_QUIT) {
        if (idbg_should_pause()) {
            idbg_show_context();
            idbg_start();
            if (idbg_get_mode() == IDBG_MODE_QUIT) break;
        }

        if (debugger_check_breakpoint(BP_TYPE_EXEC, REG.PC)) {
            idbg_handle_breakpoint(REG.PC);
            idbg_start();
            if (idbg_get_mode() == IDBG_MODE_QUIT) break;
        }

        MEM_WORD opcode = bus_read(REG.PC);
        if (opcode == 0x00) {
            printf("\nBRK instruction at $%04X\n", REG.PC);
            break;
        }

        cpu_step();
        debugger_check_watchpoints();
    }

    printf("\nExiting debugger.\n");
    debugger_cleanup();

    return 0;
}
