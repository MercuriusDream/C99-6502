#define _POSIX_C_SOURCE 200809L
#include "tui_monitor.h"
#include "cpu.h"
#include "bus.h"
#include "instructions_table.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <locale.h>

// Color pairs
#define COLOR_PANEL_BORDER 1
#define COLOR_PANEL_TITLE 2
#define COLOR_RUNNING 3
#define COLOR_STOPPED 4
#define COLOR_FLAG_SET 5
#define COLOR_FLAG_CLEAR 6
#define COLOR_HIGHLIGHT 7
#define COLOR_BUS_READ 8
#define COLOR_BUS_WRITE 9
#define COLOR_ALT_LINE 10
#define COLOR_INST_CONTROL 11    // Control flow: JMP, JSR, BRK, RTS, RTI
#define COLOR_INST_BRANCH 12     // Branch: BCC, BCS, BEQ, BNE, etc.
#define COLOR_INST_MEMORY 13     // Memory: LDA, STA, LDX, STX, etc.
#define COLOR_INST_MATH 14       // Math/Logic: ADC, SBC, AND, ORA, EOR
#define COLOR_INST_STACK 15      // Stack: PHA, PLA, PHP, PLP
#define COLOR_INST_TRANSFER 16   // Transfer: TAX, TXA, TAY, TYA, etc.

void tui_init(void) {
    // Set locale to support UTF-8 and multibyte characters
    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    // Initialize colors - clean, professional palette with dark grey background
    if (has_colors()) {
        start_color();

        // Use custom dark grey background if supported
        if (can_change_color()) {
            // Define a very dark grey (RGB values 0-1000 scale)
            init_color(COLOR_BLACK, 100, 100, 100);  // Very dark grey instead of pure black
        }

        // Use default colors to get terminal's color scheme
        use_default_colors();

        init_pair(COLOR_PANEL_BORDER, COLOR_WHITE, -1);      // Clean white borders on default bg
        init_pair(COLOR_PANEL_TITLE, COLOR_WHITE, -1);       // White titles
        init_pair(COLOR_RUNNING, COLOR_GREEN, -1);           // Green for running
        init_pair(COLOR_STOPPED, COLOR_RED, -1);             // Red for stopped
        init_pair(COLOR_FLAG_SET, COLOR_GREEN, -1);          // Green for set flags
        init_pair(COLOR_FLAG_CLEAR, COLOR_WHITE, -1);        // White for clear flags (dim)
        init_pair(COLOR_HIGHLIGHT, COLOR_BLACK, COLOR_WHITE);         // Black on white highlight
        init_pair(COLOR_BUS_READ, COLOR_WHITE, -1);          // White for reads
        init_pair(COLOR_BUS_WRITE, COLOR_WHITE, -1);         // White for writes
        init_pair(COLOR_ALT_LINE, COLOR_WHITE, -1);          // Alternate line color
        init_pair(COLOR_INST_CONTROL, COLOR_RED, -1);        // Control flow (red)
        init_pair(COLOR_INST_BRANCH, COLOR_YELLOW, -1);      // Branches (yellow)
        init_pair(COLOR_INST_MEMORY, COLOR_CYAN, -1);        // Memory ops (cyan)
        init_pair(COLOR_INST_MATH, COLOR_GREEN, -1);         // Math/logic (green)
        init_pair(COLOR_INST_STACK, COLOR_MAGENTA, -1);      // Stack ops (magenta)
        init_pair(COLOR_INST_TRANSFER, COLOR_BLUE, -1);      // Transfers (blue)
    }

    // Set background to terminal default (which should be dark grey if configured)
    bkgd(COLOR_PAIR(0));
}

void tui_cleanup(void) {
    // Clear screen before exiting
    clear();
    refresh();
    // Reset terminal to normal mode
    endwin();
    // Additional cleanup - reset terminal
    printf("\033[0m");  // Reset all attributes
    printf("\033[2J");  // Clear screen
    printf("\033[H");   // Move cursor to home
    fflush(stdout);
}

// Get color for instruction based on mnemonic
static int get_instruction_color(const char* mnemonic) {
    // Control flow instructions
    if (strncmp(mnemonic, "JMP", 3) == 0 || strncmp(mnemonic, "JSR", 3) == 0 ||
        strncmp(mnemonic, "BRK", 3) == 0 || strncmp(mnemonic, "RTS", 3) == 0 ||
        strncmp(mnemonic, "RTI", 3) == 0) {
        return COLOR_INST_CONTROL;
    }

    // Branch instructions
    if (mnemonic[0] == 'B' && (
        strncmp(mnemonic, "BCC", 3) == 0 || strncmp(mnemonic, "BCS", 3) == 0 ||
        strncmp(mnemonic, "BEQ", 3) == 0 || strncmp(mnemonic, "BNE", 3) == 0 ||
        strncmp(mnemonic, "BMI", 3) == 0 || strncmp(mnemonic, "BPL", 3) == 0 ||
        strncmp(mnemonic, "BVC", 3) == 0 || strncmp(mnemonic, "BVS", 3) == 0 ||
        strncmp(mnemonic, "BRA", 3) == 0 || strncmp(mnemonic, "BBR", 3) == 0 ||
        strncmp(mnemonic, "BBS", 3) == 0)) {
        return COLOR_INST_BRANCH;
    }

    // Memory instructions
    if (strncmp(mnemonic, "LDA", 3) == 0 || strncmp(mnemonic, "STA", 3) == 0 ||
        strncmp(mnemonic, "LDX", 3) == 0 || strncmp(mnemonic, "STX", 3) == 0 ||
        strncmp(mnemonic, "LDY", 3) == 0 || strncmp(mnemonic, "STY", 3) == 0 ||
        strncmp(mnemonic, "STZ", 3) == 0) {
        return COLOR_INST_MEMORY;
    }

    // Math/Logic instructions
    if (strncmp(mnemonic, "ADC", 3) == 0 || strncmp(mnemonic, "SBC", 3) == 0 ||
        strncmp(mnemonic, "AND", 3) == 0 || strncmp(mnemonic, "ORA", 3) == 0 ||
        strncmp(mnemonic, "EOR", 3) == 0 || strncmp(mnemonic, "CMP", 3) == 0 ||
        strncmp(mnemonic, "CPX", 3) == 0 || strncmp(mnemonic, "CPY", 3) == 0 ||
        strncmp(mnemonic, "BIT", 3) == 0 || strncmp(mnemonic, "INC", 3) == 0 ||
        strncmp(mnemonic, "DEC", 3) == 0 || strncmp(mnemonic, "INX", 3) == 0 ||
        strncmp(mnemonic, "DEX", 3) == 0 || strncmp(mnemonic, "INY", 3) == 0 ||
        strncmp(mnemonic, "DEY", 3) == 0 || strncmp(mnemonic, "ASL", 3) == 0 ||
        strncmp(mnemonic, "LSR", 3) == 0 || strncmp(mnemonic, "ROL", 3) == 0 ||
        strncmp(mnemonic, "ROR", 3) == 0) {
        return COLOR_INST_MATH;
    }

    // Stack instructions
    if (strncmp(mnemonic, "PHA", 3) == 0 || strncmp(mnemonic, "PLA", 3) == 0 ||
        strncmp(mnemonic, "PHP", 3) == 0 || strncmp(mnemonic, "PLP", 3) == 0 ||
        strncmp(mnemonic, "PHX", 3) == 0 || strncmp(mnemonic, "PLX", 3) == 0 ||
        strncmp(mnemonic, "PHY", 3) == 0 || strncmp(mnemonic, "PLY", 3) == 0) {
        return COLOR_INST_STACK;
    }

    // Transfer instructions
    if (strncmp(mnemonic, "TAX", 3) == 0 || strncmp(mnemonic, "TXA", 3) == 0 ||
        strncmp(mnemonic, "TAY", 3) == 0 || strncmp(mnemonic, "TYA", 3) == 0 ||
        strncmp(mnemonic, "TSX", 3) == 0 || strncmp(mnemonic, "TXS", 3) == 0) {
        return COLOR_INST_TRANSFER;
    }

    // Default white for everything else (NOP, CLC, SEC, CLI, SEI, etc.)
    return COLOR_PANEL_BORDER;
}

// Draw a box with title
static void draw_panel(int y, int x, int height, int width, const char* title) {
    attron(COLOR_PAIR(COLOR_PANEL_BORDER));

    // Top border
    mvaddch(y, x, ACS_ULCORNER);
    for (int i = 1; i < width - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_URCORNER);

    // Sides
    for (int i = 1; i < height - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + width - 1, ACS_VLINE);
    }

    // Bottom border
    mvaddch(y + height - 1, x, ACS_LLCORNER);
    for (int i = 1; i < width - 1; i++) {
        addch(ACS_HLINE);
    }
    addch(ACS_LRCORNER);

    attroff(COLOR_PAIR(COLOR_PANEL_BORDER));

    // Title
    if (title) {
        attron(COLOR_PAIR(COLOR_PANEL_TITLE) | A_BOLD);
        mvprintw(y, x + 2, " %s ", title);
        attroff(COLOR_PAIR(COLOR_PANEL_TITLE) | A_BOLD);
    }
}

void draw_cpu_status(MonitorState* state, int y, int x, int width, int height) {
    draw_panel(y, x, height, width, "Status");

    // Status line
    int color = state->running ? COLOR_RUNNING : COLOR_STOPPED;
    const char* status = state->running ? "RUNNING" : "STOPPED";
    const char* cpu_name = state->sys_config.cpu_variant == CPU_VARIANT_CMOS_65C02 ? "65C02" : "6502";

    // Format cycles value
    char cycles_str[16];
    if (state->total_cycles >= 1000000000000ULL) {
        snprintf(cycles_str, sizeof(cycles_str), "%.2fT", state->total_cycles / 1000000000000.0);
    } else if (state->total_cycles >= 1000000000ULL) {
        snprintf(cycles_str, sizeof(cycles_str), "%.2fG", state->total_cycles / 1000000000.0);
    } else if (state->total_cycles >= 1000000) {
        snprintf(cycles_str, sizeof(cycles_str), "%.2fM", state->total_cycles / 1000000.0);
    } else if (state->total_cycles >= 1000) {
        snprintf(cycles_str, sizeof(cycles_str), "%.1fK", state->total_cycles / 1000.0);
    } else {
        snprintf(cycles_str, sizeof(cycles_str), "%llu", (unsigned long long)state->total_cycles);
    }

    char uptime[16];
    format_uptime(state->start_time, uptime, sizeof(uptime));

    // Format memory sizes
    char ram_str[16], rom_str[16];
    if (state->sys_config.ram_size >= 1024) {
        snprintf(ram_str, sizeof(ram_str), "%uKB", state->sys_config.ram_size / 1024);
    } else {
        snprintf(ram_str, sizeof(ram_str), "%uB", state->sys_config.ram_size);
    }
    if (state->sys_config.rom_size >= 1024) {
        snprintf(rom_str, sizeof(rom_str), "%uKB", state->sys_config.rom_size / 1024);
    } else {
        snprintf(rom_str, sizeof(rom_str), "%uB", state->sys_config.rom_size);
    }

    // Line 1: State and Speed aligned
    mvprintw(y + 1, x + 2, "State: ");
    attron(COLOR_PAIR(color) | A_BOLD);
    printw("%-7s", status);
    attroff(COLOR_PAIR(color) | A_BOLD);
    printw("  Speed: %.2f MHz", state->current_mhz);

    // Line 2: Cycles and Uptime aligned
    mvprintw(y + 2, x + 2, "Cycles: %-7s  Uptime: %s", cycles_str, uptime);

    // Line 3: CPU, RAM, ROM aligned
    mvprintw(y + 3, x + 2, "CPU: %-5s  RAM: %-5s  ROM: %s", cpu_name, ram_str, rom_str);
}

void draw_system_info(MonitorState* state, int y, int x, int width, int height) {
    draw_panel(y, x, height, width, "System Configuration");

    const char* cpu_name = state->sys_config.cpu_variant == CPU_VARIANT_CMOS_65C02 ? "65C02 (CMOS)" : "6502 (NMOS)";

    mvprintw(y + 1, x + 2, "CPU: %s", cpu_name);

    mvprintw(y + 2, x + 2, "RAM: $%04X-$%04X (%u bytes)",
             state->sys_config.ram_start,
             state->sys_config.ram_start + state->sys_config.ram_size - 1,
             state->sys_config.ram_size);

    mvprintw(y + 3, x + 2, "ROM: $%04X-$%04X (%u bytes)",
             state->sys_config.rom_start,
             state->sys_config.rom_start + state->sys_config.rom_size - 1,
             state->sys_config.rom_size);

    mvprintw(y + 4, x + 2, "Vectors: RST=$%04X",
             bus_read(0xFFFC) | (bus_read(0xFFFD) << 8));
}

// Comparison function for qsort
static int compare_uint64(const void* a, const void* b) {
    uint64_t val_a = *(const uint64_t*)a;
    uint64_t val_b = *(const uint64_t*)b;
    if (val_a < val_b) return -1;
    if (val_a > val_b) return 1;
    return 0;
}

// Helper to format performance values
static void format_perf(uint64_t cycles, char* buf, size_t size) {
    if (cycles >= 1000000) {
        snprintf(buf, size, "%.2f MHz", cycles / 1000000.0);
    } else if (cycles >= 1000) {
        snprintf(buf, size, "%.1f KHz", cycles / 1000.0);
    } else {
        snprintf(buf, size, "%llu Hz", (unsigned long long)cycles);
    }
}

// Helper to draw a performance bar
static void draw_perf_bar(int row, int bar_x, int bar_width, const char* label,
                          uint64_t cycles, uint64_t max_cycles, const char* value_str, int color) {
    mvprintw(row, bar_x, "%-8s", label);
    int bar_len = (int)((double)cycles / max_cycles * bar_width);
    if (bar_len > bar_width) bar_len = bar_width;
    if (bar_len < 1) bar_len = 1;

    attron(COLOR_PAIR(color));
    for (int i = 0; i < bar_len; i++) {
        addch(ACS_CKBOARD);
    }
    attroff(COLOR_PAIR(color));

    // Pad and print value
    int padding = bar_width - bar_len;
    for (int i = 0; i < padding; i++) {
        addch(' ');
    }
    printw(" %s", value_str);
}

void draw_execution_history(MonitorState* state, int y, int x, int width, int height) {
    draw_panel(y, x, height, width, "Performance");

    // Collect valid samples into array for sorting
    uint64_t valid_samples[CYCLE_HISTORY_SIZE];
    int count = 0;
    uint64_t sum = 0;
    uint64_t max_cycles = 0;

    for (int i = 0; i < CYCLE_HISTORY_SIZE; i++) {
        if (state->cycle_history[i] > 0 && state->cycle_history[i] < 100000000) {
            valid_samples[count] = state->cycle_history[i];
            sum += state->cycle_history[i];
            count++;
            if (state->cycle_history[i] > max_cycles) {
                max_cycles = state->cycle_history[i];
            }
        }
    }

    // Get current (most recent sample)
    uint64_t current_cycles = 0;
    if (count > 0) {
        int last_idx = (state->cycle_history_idx - 1 + CYCLE_HISTORY_SIZE) % CYCLE_HISTORY_SIZE;
        current_cycles = state->cycle_history[last_idx];
        if (current_cycles == 0 || current_cycles >= 100000000) {
            current_cycles = max_cycles;
        }
    }

    if (count > 0 && max_cycles > 0) {
        uint64_t avg_cycles = sum / count;

        // Sort samples to calculate percentiles
        qsort(valid_samples, count, sizeof(uint64_t), compare_uint64);

        // Calculate 1% low (1st percentile) and 0.1% low (0.1th percentile)
        uint64_t one_percent_low = valid_samples[0];  // Default to minimum
        uint64_t point_one_percent_low = valid_samples[0];

        if (count >= 100) {
            // 1% low - worst 1% of frames
            int one_percent_idx = count / 100;
            if (one_percent_idx >= count) one_percent_idx = count - 1;
            one_percent_low = valid_samples[one_percent_idx];
        }

        if (count >= 1000) {
            // 0.1% low - worst 0.1% of frames
            int point_one_percent_idx = count / 1000;
            if (point_one_percent_idx >= count) point_one_percent_idx = count - 1;
            point_one_percent_low = valid_samples[point_one_percent_idx];
        }

        // Format all values
        char current_str[16], avg_str[16], one_low_str[16], point_one_low_str[16];
        format_perf(current_cycles, current_str, sizeof(current_str));
        format_perf(avg_cycles, avg_str, sizeof(avg_str));
        format_perf(one_percent_low, one_low_str, sizeof(one_low_str));
        format_perf(point_one_percent_low, point_one_low_str, sizeof(point_one_low_str));

        // Calculate bar dimensions
        int bar_width = width - 24;  // Leave room for label and value
        int bar_y = y + 1;
        int bar_x = x + 2;

        // Draw all performance bars
        draw_perf_bar(bar_y,     bar_x, bar_width, "Curr",  current_cycles,        max_cycles, current_str,       COLOR_RUNNING | A_BOLD);
        draw_perf_bar(bar_y + 1, bar_x, bar_width, "Avg",   avg_cycles,            max_cycles, avg_str,           COLOR_PANEL_TITLE);
        draw_perf_bar(bar_y + 2, bar_x, bar_width, "1%",    one_percent_low,       max_cycles, one_low_str,       COLOR_BUS_READ);
        draw_perf_bar(bar_y + 3, bar_x, bar_width, ".1%",   point_one_percent_low, max_cycles, point_one_low_str, COLOR_BUS_WRITE);
    } else {
        // Show 0.00 MHz for all metrics when no data yet
        int bar_y = y + 1;
        int bar_x = x + 2;

        mvprintw(bar_y,     bar_x, "%-8s 0.00 MHz", "Curr");
        mvprintw(bar_y + 1, bar_x, "%-8s 0.00 MHz", "Avg");
        mvprintw(bar_y + 2, bar_x, "%-8s 0.00 MHz", "1%");
        mvprintw(bar_y + 3, bar_x, "%-8s 0.00 MHz", ".1%");
    }
}

void draw_bus_activity(MonitorState* state, int y, int x, int width, int height) {
    draw_panel(y, x, height, width, "Bus Activity");

    // Bus state
    const char* state_str = "IDLE";
    int color = COLOR_WHITE;

    if (state->bus.state == BUS_READ) {
        state_str = "READ";
        color = COLOR_BUS_READ;
    } else if (state->bus.state == BUS_WRITE) {
        state_str = "WRITE";
        color = COLOR_BUS_WRITE;
    }

    mvprintw(y + 1, x + 2, "State: ");
    attron(COLOR_PAIR(color) | A_BOLD);
    printw("%s", state_str);
    attroff(COLOR_PAIR(color) | A_BOLD);

    printw("   Addr: $%04X  Data: $%02X", state->bus.addr, state->bus.data);

    // Target region
    mvprintw(y + 2, x + 2, "Target: %s", state->bus.target);

    // Last read
    mvprintw(y + 3, x + 2, "Last Read:  $%04X => $%02X",
             state->bus.last_read_addr, state->bus.last_read_data);

    // Last write
    mvprintw(y + 4, x + 2, "Last Write: $%04X <= $%02X",
             state->bus.last_write_addr, state->bus.last_write_data);
}

void draw_registers(MonitorState* state, int y, int x, int width, int height) {
    draw_panel(y, x, height, width, "Registers");

    // Compact register display
    mvprintw(y + 1, x + 2, "PC: $%04X  SP: $%02X   A: $%02X  X: $%02X",
             REG.PC, REG.S, REG.A, REG.X);
    mvprintw(y + 2, x + 2, "P:  $%02X    Y:  $%02X   ",
             REG.P, REG.Y);

    // Flags with letter highlighting (Bold=Set, Dim=Clear)
    mvprintw(y + 3, x + 2, "Flags: [");

    const MEM_WORD flags[] = {FLAG_N, FLAG_V, FLAG_U, FLAG_B, FLAG_D, FLAG_I, FLAG_Z, FLAG_C};
    const char flag_chars[] = "NV-BDIZC";

    for (int i = 0; i < 8; i++) {
        if (i > 0) addch(' ');
        if (REG.P & flags[i]) {
            attron(COLOR_PAIR(COLOR_FLAG_SET) | A_BOLD);
            addch(flag_chars[i]);
            attroff(COLOR_PAIR(COLOR_FLAG_SET) | A_BOLD);
        } else {
            attron(COLOR_PAIR(COLOR_FLAG_CLEAR) | A_DIM);
            addch(tolower(flag_chars[i]));
            attroff(COLOR_PAIR(COLOR_FLAG_CLEAR) | A_DIM);
        }
    }
    printw(" ]");
}

void draw_stack_depth(MonitorState* state, int y, int x, int width, int height) {
    draw_panel(y, x, height, width, "Stack Depth");

    // Calculate stack usage
    int stack_used = 0xFF - REG.S;
    int stack_free = REG.S;
    double stack_percent = (double)stack_used / 256.0 * 100.0;

    // Stack pointer info
    mvprintw(y + 1, x + 2, "SP: $%02X  Used: %d/256", REG.S, stack_used);

    // Draw progress bar
    int bar_width = width - 4;
    int filled = (int)(stack_percent / 100.0 * bar_width);

    mvprintw(y + 2, x + 2, "[");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            if (stack_percent > 90) {
                attron(COLOR_PAIR(COLOR_STOPPED));
            } else if (stack_percent > 75) {
                attron(COLOR_PAIR(COLOR_PANEL_TITLE));
            } else {
                attron(COLOR_PAIR(COLOR_RUNNING));
            }
            addch(ACS_VLINE);
            if (stack_percent > 90) {
                attroff(COLOR_PAIR(COLOR_STOPPED));
            } else if (stack_percent > 75) {
                attroff(COLOR_PAIR(COLOR_PANEL_TITLE));
            } else {
                attroff(COLOR_PAIR(COLOR_RUNNING));
            }
        } else {
            addch(' ');
        }
    }
    printw("]");

    mvprintw(y + 3, x + 2, "%d%% Free  Range: $0100-$01FF", (int)((double)stack_free / 256.0 * 100.0));
}

void draw_disassembly(MonitorState* state, int y, int x, int width, int height) {
    draw_panel(y, x, height, width, "Disassembly");

    MEM_TWO_WORDS addr = REG.PC;
    int max_lines = height - 2;

    // Only draw as many lines as we have space for
    for (int line = 0; line < max_lines; line++) {
        MEM_WORD opcode = bus_read(addr);
        const INST* meta = &INST_TABLE[opcode >> 4][opcode & 0x0F];

        int operand_size = 0;
        if (meta->ADDR == ADDR_IMM || meta->ADDR == ADDR_ZP || meta->ADDR == ADDR_ZPX ||
            meta->ADDR == ADDR_ZPY || meta->ADDR == ADDR_INDX || meta->ADDR == ADDR_INDY ||
            meta->ADDR == ADDR_REL || meta->ADDR == ADDR_ZP_IND || meta->ADDR == ADDR_ZPREL) {
            operand_size = 1;
        } else if (meta->ADDR == ADDR_ABS || meta->ADDR == ADDR_ABSX || meta->ADDR == ADDR_ABSY ||
                   meta->ADDR == ADDR_IND || meta->ADDR == ADDR_ABS_IND_X) {
            operand_size = 2;
        }

        // Read operand bytes
        MEM_WORD operand1 = 0, operand2 = 0;
        if (operand_size >= 1) operand1 = bus_read(addr + 1);
        if (operand_size >= 2) operand2 = bus_read(addr + 2);

        // Highlight current instruction, or use instruction category colors
        if (addr == REG.PC) {
            attron(COLOR_PAIR(COLOR_HIGHLIGHT));
            mvprintw(y + 1 + line, x + 2, ">%04X ", addr);
        } else {
            mvprintw(y + 1 + line, x + 2, " %04X ", addr);
        }

        // Print opcode bytes (up to 3 bytes: opcode + operands)
        if (operand_size == 0) {
            printw("%02X       ", opcode);
        } else if (operand_size == 1) {
            printw("%02X %02X    ", opcode, operand1);
        } else if (operand_size == 2) {
            printw("%02X %02X %02X ", opcode, operand1, operand2);
        }

        // Color the mnemonic based on instruction type
        if (addr == REG.PC) {
            printw("%s", meta->CMD);
            attroff(COLOR_PAIR(COLOR_HIGHLIGHT));
        } else {
            int inst_color = get_instruction_color(meta->CMD);
            attron(COLOR_PAIR(inst_color));
            printw("%s", meta->CMD);
            attroff(COLOR_PAIR(inst_color));
        }

        addr += 1 + operand_size;
    }
}

void draw_memory_watch(MonitorState* state, int y, int x, int width, int height) {
    draw_panel(y, x, height, width, "Memory");

    // Hexdump format: Address + 8 bytes + ASCII
    int max_lines = height - 2;
    MEM_TWO_WORDS base_addr = 0x0000;  // Could make this configurable

    // Use all available lines efficiently
    int bus_lines = 2;  // Lines for last read/write
    int mem_lines = max_lines - bus_lines;
    if (mem_lines < 0) mem_lines = 0;

    for (int line = 0; line < mem_lines; line++) {
        MEM_TWO_WORDS addr = base_addr + (line * 8);
        mvprintw(y + 1 + line, x + 2, "$%04X: ", addr);

        // Hex bytes
        for (int i = 0; i < 8; i++) {
            MEM_WORD byte = bus_read(addr + i);
            printw("%02X ", byte);
        }

        printw(" ");

        // ASCII representation (only printable ASCII chars)
        for (int i = 0; i < 8; i++) {
            MEM_WORD byte = bus_read(addr + i);
            // Only show standard printable ASCII (32-126), everything else is '.'
            if (byte >= 0x20 && byte <= 0x7E) {
                addch((char)byte);
            } else {
                addch('.');
            }
        }
    }

    // Show last bus activity at bottom
    if (max_lines >= 2) {
        mvprintw(y + height - 3, x + 2, "Last Read:  $%04X => $%02X",
                 state->bus.last_read_addr, state->bus.last_read_data);
        mvprintw(y + height - 2, x + 2, "Last Write: $%04X <= $%02X",
                 state->bus.last_write_addr, state->bus.last_write_data);
    }
}

void draw_instruction_log(MonitorState* state, int y, int x, int width, int height) {
    draw_panel(y, x, height, width, "Instructions");

    mvprintw(y + 1, x + 2, "%-4s %-3s %s", "Addr", "Ins", "State");

    int display_count = (height - 3 < INSTRUCTION_LOG_DISPLAY) ? height - 3 : INSTRUCTION_LOG_DISPLAY;

    for (int i = 0; i < display_count && i < state->instruction_log_count; i++) {
        int idx = (state->instruction_log_idx - 1 - i + INSTRUCTION_LOG_SIZE) % INSTRUCTION_LOG_SIZE;
        InstructionLogEntry* entry = &state->instruction_log[idx];

        // Print with instruction color
        int inst_color = get_instruction_color(entry->mnemonic);

        mvprintw(y + 2 + i, x + 2, "%04X ", entry->addr);

        attron(COLOR_PAIR(inst_color));
        printw("%-3s", entry->mnemonic);
        attroff(COLOR_PAIR(inst_color));

        printw(" %s", entry->state_change);
    }
}

void draw_help_bar(int y, int x, int width) {
    attron(A_REVERSE);
    mvprintw(y, x, "%-*s", width,
             " [S] Step  [C] Continue  [B] Break  [R] Reset  [M] Memory  [Q] Quit  [?] Help");
    attroff(A_REVERSE);
}

void tui_draw(MonitorState* state) {
    // Use erase() instead of clear() to prevent flicker
    erase();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int y_pos = 0;

    // Row 1: CPU Status (left) and Registers (right)
    // Equal width panels - 50/50 split
    int top_left_width = max_x / 2;
    int top_right_width = max_x - top_left_width;
    draw_cpu_status(state, y_pos, 0, top_left_width, 5);
    draw_registers(state, y_pos, top_left_width, top_right_width, 5);
    y_pos += 5;

    // Row 2: Disassembly (left) and Memory View (right) - COMBINED
    int middle_height = 7;  // Reduced from 10 to 7 for more compact display
    draw_disassembly(state, y_pos, 0, top_left_width, middle_height);
    draw_memory_watch(state, y_pos, top_left_width, top_right_width, middle_height);
    y_pos += middle_height;

    // Row 3: Instruction Log (left, matches top panels) and Performance History (right, matches top panels)
    int bottom_height = max_y - y_pos - 1;  // Leave room for help bar
    if (bottom_height > 5) {
        draw_instruction_log(state, y_pos, 0, top_left_width, bottom_height);
        draw_execution_history(state, y_pos, top_left_width, top_right_width, bottom_height);
    }

    // Help bar at bottom
    draw_help_bar(max_y - 1, 0, max_x);

    refresh();
}

void tui_handle_input(MonitorState* state) {
    int ch = getch();

    if (ch == ERR) return;

    // Ignore non-ASCII input and flush any remaining multibyte sequences
    if (ch > 127 || ch < 0) {
        // Flush remaining input buffer to clear multibyte sequences
        flushinp();
        return;
    }

    switch (ch) {
        case 'q':
        case 'Q':
            state->should_quit = 1;
            break;
        case 's':
        case 'S':
            state->stepping = 1;
            state->running = 0;
            break;
        case 'c':
        case 'C':
            state->running = 1;
            state->stepping = 0;
            break;
        case 'b':
        case 'B':
            state->running = 0;
            break;
        case 'r':
        case 'R':
            cpu_reset();
            state->reset_pending = 1;
            break;
        default:
            // Flush input buffer for any unrecognized input
            flushinp();
            break;
    }
}

// Monitor state functions
void monitor_state_init(MonitorState* state) {
    memset(state, 0, sizeof(MonitorState));
    clock_gettime(CLOCK_MONOTONIC, &state->start_time);
    state->running = 0;
    state->stepping = 0;
    state->should_quit = 0;

    // Initialize some default watch addresses
    monitor_add_watch(state, 0x0000);
    monitor_add_watch(state, 0x0200);
}

void monitor_state_update(MonitorState* state) {
    state->total_cycles = CYCLES;

    // Update cycle history every 10ms for responsive performance tracking
    static struct timespec last_update = {0};
    static uint64_t last_cycles = 0;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (last_update.tv_sec == 0) {
        last_update = now;
        last_cycles = state->total_cycles;
    }

    // Calculate elapsed time in milliseconds
    long elapsed_ms = (now.tv_sec - last_update.tv_sec) * 1000 +
                      (now.tv_nsec - last_update.tv_nsec) / 1000000;

    // Store in history every 10ms
    if (elapsed_ms >= 10) {
        uint64_t cycles_delta = state->total_cycles - last_cycles;
        uint64_t cycles_per_second = (cycles_delta * 1000) / elapsed_ms;

        state->cycle_history[state->cycle_history_idx] = cycles_per_second;
        state->cycle_history_idx = (state->cycle_history_idx + 1) % CYCLE_HISTORY_SIZE;
        last_update = now;
        last_cycles = state->total_cycles;
    }

    // Calculate average MHz from last 30 samples (300ms) for smooth, stable display
    uint64_t sum = 0;
    int count = 0;
    int samples_to_avg = 30;

    for (int i = 0; i < samples_to_avg && i < CYCLE_HISTORY_SIZE; i++) {
        int idx = (state->cycle_history_idx - 1 - i + CYCLE_HISTORY_SIZE) % CYCLE_HISTORY_SIZE;
        if (state->cycle_history[idx] > 0 && state->cycle_history[idx] < 100000000) {
            sum += state->cycle_history[idx];
            count++;
        }
    }

    if (count > 0) {
        uint64_t avg_cycles_per_sec = sum / count;
        state->current_mhz = avg_cycles_per_sec / 1e6;
    } else {
        state->current_mhz = 0.0;
    }

    // Update memory watches
    for (int i = 0; i < state->watch_count; i++) {
        MEM_WORD new_value = bus_read(state->watches[i].addr);
        if (new_value != state->watches[i].value) {
            state->watches[i].prev_value = state->watches[i].value;
            state->watches[i].value = new_value;
            state->watches[i].changed = 1;
        } else {
            state->watches[i].changed = 0;
        }
    }
}

void monitor_log_instruction(MonitorState* state, MEM_TWO_WORDS pc, MEM_WORD opcode) {
    InstructionLogEntry* entry = &state->instruction_log[state->instruction_log_idx];

    entry->cycle = state->total_cycles;
    entry->addr = pc;
    entry->opcode = opcode;

    const INST* meta = &INST_TABLE[opcode >> 4][opcode & 0x0F];
    strncpy(entry->mnemonic, meta->CMD, sizeof(entry->mnemonic) - 1);
    entry->mnemonic[sizeof(entry->mnemonic) - 1] = '\0';

    // Simple state change tracking
    snprintf(entry->state_change, sizeof(entry->state_change),
             "A=$%02X X=$%02X Y=$%02X", REG.A, REG.X, REG.Y);

    clock_gettime(CLOCK_MONOTONIC, &entry->timestamp);

    state->instruction_log_idx = (state->instruction_log_idx + 1) % INSTRUCTION_LOG_SIZE;
    if (state->instruction_log_count < INSTRUCTION_LOG_SIZE) {
        state->instruction_log_count++;
    }
}

void monitor_update_bus(MonitorState* state, BusState bus_state, MEM_TWO_WORDS addr, MEM_WORD data) {
    state->bus.state = bus_state;
    state->bus.addr = addr;
    state->bus.data = data;

    if (bus_state == BUS_READ) {
        state->bus.last_read_addr = addr;
        state->bus.last_read_data = data;
    } else if (bus_state == BUS_WRITE) {
        state->bus.last_write_addr = addr;
        state->bus.last_write_data = data;
    }

    // Determine target region
    if (addr < state->sys_config.ram_start + state->sys_config.ram_size) {
        strncpy(state->bus.target, "RAM", sizeof(state->bus.target) - 1);
        state->bus.target[sizeof(state->bus.target) - 1] = '\0';
    } else if (addr >= state->sys_config.rom_start) {
        strncpy(state->bus.target, "ROM", sizeof(state->bus.target) - 1);
        state->bus.target[sizeof(state->bus.target) - 1] = '\0';
    } else {
        strncpy(state->bus.target, "Unmapped", sizeof(state->bus.target) - 1);
        state->bus.target[sizeof(state->bus.target) - 1] = '\0';
    }
}

void monitor_add_watch(MonitorState* state, MEM_TWO_WORDS addr) {
    if (state->watch_count < MEMORY_WATCH_SIZE) {
        state->watches[state->watch_count].addr = addr;
        state->watches[state->watch_count].value = bus_read(addr);
        state->watches[state->watch_count].prev_value = state->watches[state->watch_count].value;
        state->watches[state->watch_count].changed = 0;
        state->watch_count++;
    }
}

void format_uptime(struct timespec start, char* buf, size_t bufsize) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long elapsed = now.tv_sec - start.tv_sec;
    int hours = elapsed / 3600;
    int minutes = (elapsed % 3600) / 60;
    int seconds = elapsed % 60;

    snprintf(buf, bufsize, "%02d:%02d:%02d", hours, minutes, seconds);
}

double calculate_mhz(uint64_t cycles, struct timespec start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    double elapsed = (now.tv_sec - start.tv_sec) +
                     (now.tv_nsec - start.tv_nsec) / 1e9;

    if (elapsed == 0) return 0.0;

    return (cycles / elapsed) / 1e6;
}
