#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "instructions_table.h"
#include "instructions_implementation.h"
#include "memory.h"
#include "trace.h"
#include "loader.h"
#include "tui_monitor.h"
#include "logging.h"

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
    ARG_MONITOR
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
    } else if (strcmp(arg, "-m") == 0 || strcmp(arg, "--monitor") == 0) {
        return ARG_MONITOR;
    }
    return ARG_UNKNOWN;
}

// Forward declaration
static void run_monitor_mode(MEM_TWO_WORDS ram_start, MEM_TWO_WORDS ram_size,
                             MEM_TWO_WORDS rom_start, MEM_TWO_WORDS rom_size,
                             CPU_VARIANT cpu_variant);

int main(int argc, char** argv) {
    int enable_trace = 0;
    int enable_monitor = 0;
    const char* rom_file = NULL;
    MEM_TWO_WORDS rom_addr = 0x8000;
    CPU_VARIANT cpu_variant = CPU_VARIANT_NMOS_6502;  // Default to NMOS

    // Memory configuration defaults
    MEM_TWO_WORDS ram_start = 0x0000;
    MEM_TWO_WORDS ram_size = 0x8000;  // 32KB
    MEM_TWO_WORDS rom_start = 0x8000;
    MEM_TWO_WORDS rom_size = 0x8000;  // 32KB

    // Parse command line arguments
    for (int i=1; i<argc; i++) {
        ArgType arg_type = parse_arg(argv[i]);

        switch (arg_type) {
            case ARG_TRACE:
                enable_trace = 1;
                break;

            case ARG_MONITOR:
                enable_monitor = 1;
                break;

            case ARG_FILE:
                if (i+1 < argc) {
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
                        char msg[MAX_LOG_LENGTH];
                        snprintf(msg, sizeof(msg), "Unknown CPU variant: %s", argv[i]);
                        logging(msg, 0, 1, 1, NULL, LOG_ERROR);
                        return 1;
                    }
                }
                break;

            case ARG_UNKNOWN:
            default:
                {
                    char msg[MAX_LOG_LENGTH];
                    snprintf(msg, sizeof(msg), "Unknown argument: %s", argv[i]);
                    logging(msg, 0, 1, 1, NULL, LOG_WARN);
                }
                break;
        }
    }

    // Set CPU variant
    cpu_set_variant(cpu_variant);

    char msg[MAX_LOG_LENGTH];
    snprintf(msg, sizeof(msg), "C99-6502... Running in %s MODE",
             cpu_variant == CPU_VARIANT_CMOS_65C02 ? "CMOS 65C02" : "NMOS 6502");
    logging(msg, 0, 1, 0, NULL, LOG_INFO);

    mem_region_clear(); // Memory Cleanup

    logging("RAM allocation...", 0, 0, 0, NULL, LOG_INFO);
    if (mem_region_add_ram(ram_start, ram_size) != 0) {
        logging(" FAIL!", 0, 1, 0, NULL, LOG_ERROR);
        return 1;
    }
    snprintf(msg, sizeof(msg), " %uB OK ($%04X-$%04X)", ram_size, ram_start, ram_start+ram_size-1);
    logging(msg, 0, 1, 0, NULL, LOG_INFO);

    logging("ROM allocation...", 0, 0, 0, NULL, LOG_INFO);
    if (mem_region_add_rom(rom_start, rom_size) != 0) {
        logging(" FAIL!", 0, 1, 0, NULL, LOG_ERROR);
        return 1;
    }
    snprintf(msg, sizeof(msg), " %uB OK ($%04X-$%04X)", rom_size, rom_start, rom_start+rom_size-1);
    logging(msg, 0, 1, 0, NULL, LOG_INFO);

    logging("ROM loading...", 0, 0, 0, NULL, LOG_INFO);
    if (rom_file) { // Loading the ROM file
        if (load_bin_region(rom_file, rom_addr) == 0) {
            snprintf(msg, sizeof(msg), " OK (%s at $%04X)", rom_file, rom_addr);
            logging(msg, 0, 1, 0, NULL, LOG_INFO);
        } else {
            logging(" FAIL!", 0, 1, 0, NULL, LOG_ERROR);
            return 1;
        }
    } else {
        // Load sample ROM: Simple loop that increments registers
        MEM_WORD sample_rom[] = {
            0xA9, 0x42,        // LDA #$42
            0x8D, 0x00, 0x02,  // STA $0200
            0xE8,              // INX
            0xC8,              // INY
            0xEE, 0x00, 0x02,  // INC $0200
            0x4C, 0x00, 0x80   // JMP $8000 (loop back to start)
        };
        mem_region_load(rom_addr, sample_rom, sizeof(sample_rom));
        snprintf(msg, sizeof(msg), " OK ($%04X)", rom_addr);
        logging(msg, 0, 1, 0, NULL, LOG_INFO);
    }

    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, rom_addr); // Setting the Memory Vector
    snprintf(msg, sizeof(msg), "Vector resetting... OK ($%04X)", rom_addr);
    logging(msg, 0, 1, 0, NULL, LOG_INFO);

    mem_region_init(); // Initialize the Memory region
    logging("Connecting memory bus... OK", 0, 1, 0, NULL, LOG_INFO);

    cpu_reset(); // Reset the CPU state
    logging("CPU resetting... OK", 0, 1, 0, NULL, LOG_INFO);
    snprintf(msg, sizeof(msg), "PC: $%04X, SP: $%02X, P: $%02X, A: $%02X, X: $%02X, Y: $%02X",
             REG.PC, REG.S, REG.P, REG.A, REG.X, REG.Y);
    logging(msg, 0, 1, 0, NULL, LOG_INFO);

    if (enable_trace) { // Enable Trace if needed
        trace_set_enabled(1);
        logging("Trace : True", 0, 1, 0, NULL, LOG_INFO);
    }

    // Run in monitor mode if requested
    if (enable_monitor) {
        logging("Starting TUI monitor...", 0, 1, 0, NULL, LOG_INFO);
        sleep(1);
        run_monitor_mode(ram_start, ram_size, rom_start, rom_size, cpu_variant);
        return 0;
    }

    // Normal execution mode
    logging("All set...", 0, 1, 0, NULL, LOG_INFO);
    for (int cycle_cnt=1; cycle_cnt<=CPU_TEST_RUN_LIMIT; cycle_cnt++) {
        cpu_step();
        if (!(REG.PC)) {
            snprintf(msg, sizeof(msg), "BRK at %d... Execution terminated.", cycle_cnt);
            logging(msg, 0, 1, 0, NULL, LOG_INFO);
            break;
        }
    }

    logging("\nExecution complete.", 0, 1, 0, NULL, LOG_INFO);
    snprintf(msg, sizeof(msg), "PC: $%04X, SP: $%02X, P: $%02X, A: $%02X, X: $%02X, Y: $%02X",
             REG.PC, REG.S, REG.P, REG.A, REG.X, REG.Y);
    logging(msg, 0, 1, 0, NULL, LOG_INFO);
    snprintf(msg, sizeof(msg), "Memory at $0200: $%02X", bus_read(0x0200));
    logging(msg, 0, 1, 0, NULL, LOG_INFO);

    return 0;
}

// TUI Monitor mode implementation
#define UPDATE_INTERVAL_MS 10
#define FRAME_TIME_US (UPDATE_INTERVAL_MS * 1000)
#define INSTRUCTIONS_PER_FRAME 20000  // ~2 MHz at 10ms updates (20k instructions per 10ms = 2M per second)

static void run_monitor_mode(MEM_TWO_WORDS ram_start, MEM_TWO_WORDS ram_size,
                             MEM_TWO_WORDS rom_start, MEM_TWO_WORDS rom_size,
                             CPU_VARIANT cpu_variant) {
    MonitorState monitor;

    // Check terminal size
    tui_init();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    tui_cleanup();

    if (max_x < SCREEN_MIN_WIDTH || max_y < SCREEN_MIN_HEIGHT) {
        char err_msg[MAX_LOG_LENGTH];
        snprintf(err_msg, sizeof(err_msg), "Error: Terminal too small. Need at least %dx%d, got %dx%d",
                 SCREEN_MIN_WIDTH, SCREEN_MIN_HEIGHT, max_x, max_y);
        logging(err_msg, 0, 1, 1, NULL, LOG_ERROR);
        return;
    }

    tui_init();
    monitor_state_init(&monitor);

    // Set system configuration
    monitor.sys_config.ram_start = ram_start;
    monitor.sys_config.ram_size = ram_size;
    monitor.sys_config.rom_start = rom_start;
    monitor.sys_config.rom_size = rom_size;
    monitor.sys_config.cpu_variant = cpu_variant;

    // Add some memory watches
    monitor_add_watch(&monitor, 0x0000);
    monitor_add_watch(&monitor, 0x0200);
    monitor_add_watch(&monitor, 0x0201);

    while (!monitor.should_quit) {
        struct timespec frame_start;
        clock_gettime(CLOCK_MONOTONIC, &frame_start);

        // Handle input
        tui_handle_input(&monitor);

        // Execute CPU
        if (monitor.running || monitor.stepping) {
            for (int i = 0; i < INSTRUCTIONS_PER_FRAME && (monitor.running || monitor.stepping); i++) {
                MEM_TWO_WORDS pc = REG.PC;
                MEM_WORD opcode = bus_read(pc);

                // Only log every Nth instruction when running to reduce overhead
                if (monitor.stepping || i % 10 == 0) {
                    monitor_log_instruction(&monitor, pc, opcode);
                    monitor_update_bus(&monitor, BUS_READ, pc, opcode);
                }

                cpu_step();

                if (monitor.stepping) {
                    monitor.stepping = 0;
                    monitor.running = 0;
                    break;
                }
            }
            monitor_update_bus(&monitor, BUS_IDLE, 0, 0);
        }

        // Update monitor state
        monitor_state_update(&monitor);

        // Draw TUI
        tui_draw(&monitor);

        // Frame rate limiting
        struct timespec frame_end;
        clock_gettime(CLOCK_MONOTONIC, &frame_end);

        long elapsed_us = (frame_end.tv_sec - frame_start.tv_sec) * 1000000 +
                         (frame_end.tv_nsec - frame_start.tv_nsec) / 1000;

        if (elapsed_us < FRAME_TIME_US) {
            usleep(FRAME_TIME_US - elapsed_us);
        }
    }

    tui_cleanup();

    logging("\nMonitor exited.", 0, 1, 0, NULL, LOG_INFO);
    char cycles_msg[MAX_LOG_LENGTH];
    snprintf(cycles_msg, sizeof(cycles_msg), "Total cycles executed: %llu", (unsigned long long)monitor.total_cycles);
    logging(cycles_msg, 0, 1, 0, NULL, LOG_INFO);
}
