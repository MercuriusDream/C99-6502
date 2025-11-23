#include "debugger.h"
#include "cpu.h"
#include "bus.h"
#include "instructions_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline char* _strdup(const char* dup_src_str) {
    char* dup_obj_str;
    if (!(dup_obj_str = malloc(strlen(dup_src_str)+1)) || !strcpy(dup_obj_str, dup_src_str)) return NULL;
    return dup_obj_str;
}

// Breakpoint and watchpoint arrays
static BREAKPOINT breakpoints[MAX_BREAKPOINTS];
static WATCHPOINT watchpoints[MAX_WATCHPOINTS];
static int breakpoint_count = 0;
static int watchpoint_count = 0;

// Profiling data
static PROFILING_DATA prof_data;
static int profiling_enabled = 0;

// Interactive debugger state
static int interactive_mode = 0;
static int debugger_paused = 0;


// Initialization and Cleanup


void debugger_init(void) {
    memset(breakpoints, 0, sizeof(breakpoints));
    memset(watchpoints, 0, sizeof(watchpoints));
    breakpoint_count = 0;
    watchpoint_count = 0;
    profiler_init();
}

void debugger_cleanup(void) {
    // Free any allocated labels
    for (int i = 0; i < breakpoint_count; i++) {
        if (breakpoints[i].label) {
            free((void*)breakpoints[i].label);
        }
    }
    for (int i = 0; i < watchpoint_count; i++) {
        if (watchpoints[i].label) {
            free((void*)watchpoints[i].label);
        }
    }
}


// Breakpoint Management


int debugger_add_breakpoint(BP_TYPE type, MEM_TWO_WORDS addr, const char* label) {
    if (breakpoint_count >= MAX_BREAKPOINTS) {
        printf("[Debugger] Error: Maximum breakpoints (%d) reached\n", MAX_BREAKPOINTS);
        return -1;
    }

    BREAKPOINT* bp = &breakpoints[breakpoint_count];
    bp->enabled = 1;
    bp->type = type;
    bp->addr = addr;
    if (label) {
        bp->label = _strdup(label);
        if (!bp->label) {
            printf("[Debugger] Warning: Failed to allocate memory for breakpoint label\n");
        }
    } else {
        bp->label = NULL;
    }

    printf("[Debugger] Breakpoint #%d set at $%04X", breakpoint_count, addr);
    if (label) {
        printf(" (%s)", label);
    }
    printf("\n");

    return breakpoint_count++;
}

int debugger_remove_breakpoint(int index) {
    if (index < 0 || index >= breakpoint_count) {
        return -1;
    }

    if (breakpoints[index].label) {
        free((void*)breakpoints[index].label);
    }

    // Shift remaining breakpoints
    for (int i = index; i < breakpoint_count - 1; i++) {
        breakpoints[i] = breakpoints[i + 1];
    }
    breakpoint_count--;

    return 0;
}

int debugger_enable_breakpoint(int index, int enabled) {
    if (index < 0 || index >= breakpoint_count) {
        return -1;
    }
    breakpoints[index].enabled = enabled;
    return 0;
}

int debugger_check_breakpoint(BP_TYPE type, MEM_TWO_WORDS addr) {
    for (int i = 0; i < breakpoint_count; i++) {
        if (!breakpoints[i].enabled) continue;

        if (breakpoints[i].addr == addr) {
            // Check type match
            if (breakpoints[i].type == type || breakpoints[i].type == BP_TYPE_ACCESS) {
                printf("[Debugger] Breakpoint #%d hit at $%04X", i, addr);
                if (breakpoints[i].label) {
                    printf(" (%s)", breakpoints[i].label);
                }
                printf("\n");
                return 1;
            }
        }
    }
    return 0;
}

void debugger_list_breakpoints(void) {
    printf("[Debugger] Breakpoints (%d/%d):\n", breakpoint_count, MAX_BREAKPOINTS);
    for (int i = 0; i < breakpoint_count; i++) {
        BREAKPOINT* bp = &breakpoints[i];
        const char* type_str = "EXEC";
        if (bp->type == BP_TYPE_READ) type_str = "READ";
        else if (bp->type == BP_TYPE_WRITE) type_str = "WRITE";
        else if (bp->type == BP_TYPE_ACCESS) type_str = "ACCESS";

        printf("  #%d: [%s] $%04X %s", i,
               bp->enabled ? "ENABLED" : "DISABLED",
               bp->addr, type_str);
        if (bp->label) {
            printf(" - %s", bp->label);
        }
        printf("\n");
    }
}

void debugger_clear_breakpoints(void) {
    for (int i = 0; i < breakpoint_count; i++) {
        if (breakpoints[i].label) {
            free((void*)breakpoints[i].label);
        }
    }
    memset(breakpoints, 0, sizeof(breakpoints));
    breakpoint_count = 0;
    printf("[Debugger] All breakpoints cleared\n");
}


// Watchpoint Management


int debugger_add_watchpoint(MEM_TWO_WORDS addr, const char* label) {
    if (watchpoint_count >= MAX_WATCHPOINTS) {
        printf("[Debugger] Error: Maximum watchpoints (%d) reached\n", MAX_WATCHPOINTS);
        return -1;
    }

    WATCHPOINT* wp = &watchpoints[watchpoint_count];
    wp->enabled = 1;
    wp->addr = addr;
    wp->old_value = bus_read(addr);
    if (label) {
        wp->label = _strdup(label);
        if (!wp->label) {
            printf("[Debugger] Warning: Failed to allocate memory for watchpoint label\n");
        }
    } else {
        wp->label = NULL;
    }

    printf("[Debugger] Watchpoint #%d set at $%04X (value: $%02X)",
           watchpoint_count, addr, wp->old_value);
    if (label) {
        printf(" (%s)", label);
    }
    printf("\n");

    return watchpoint_count++;
}

int debugger_remove_watchpoint(int index) {
    if (index < 0 || index >= watchpoint_count) {
        return -1;
    }

    if (watchpoints[index].label) {
        free((void*)watchpoints[index].label);
    }

    for (int i = index; i < watchpoint_count - 1; i++) {
        watchpoints[i] = watchpoints[i + 1];
    }
    watchpoint_count--;

    return 0;
}

int debugger_enable_watchpoint(int index, int enabled) {
    if (index < 0 || index >= watchpoint_count) {
        return -1;
    }
    watchpoints[index].enabled = enabled;
    return 0;
}

void debugger_check_watchpoints(void) {
    for (int i = 0; i < watchpoint_count; i++) {
        if (!watchpoints[i].enabled) continue;

        MEM_WORD new_value = bus_read(watchpoints[i].addr);
        if (new_value != watchpoints[i].old_value) {
            printf("[Debugger] Watchpoint #%d triggered at $%04X: $%02X -> $%02X",
                   i, watchpoints[i].addr, watchpoints[i].old_value, new_value);
            if (watchpoints[i].label) {
                printf(" (%s)", watchpoints[i].label);
            }
            printf("\n");
            watchpoints[i].old_value = new_value;
        }
    }
}

void debugger_list_watchpoints(void) {
    printf("[Debugger] Watchpoints (%d/%d):\n", watchpoint_count, MAX_WATCHPOINTS);
    for (int i = 0; i < watchpoint_count; i++) {
        WATCHPOINT* wp = &watchpoints[i];
        MEM_WORD current = bus_read(wp->addr);
        printf("  #%d: [%s] $%04X = $%02X",
               i,
               wp->enabled ? "ENABLED" : "DISABLED",
               wp->addr, current);
        if (wp->label) {
            printf(" - %s", wp->label);
        }
        printf("\n");
    }
}

void debugger_clear_watchpoints(void) {
    for (int i = 0; i < watchpoint_count; i++) {
        if (watchpoints[i].label) {
            free((void*)watchpoints[i].label);
        }
    }
    memset(watchpoints, 0, sizeof(watchpoints));
    watchpoint_count = 0;
    printf("[Debugger] All watchpoints cleared\n");
}


// Profiling Functions


void profiler_init(void) {
    memset(&prof_data, 0, sizeof(prof_data));
    profiling_enabled = 1;
}

void profiler_record_instruction(MEM_TWO_WORDS pc, MEM_WORD opcode, unsigned int cycles) {
    if (!profiling_enabled) return;

    prof_data.total_cycles += cycles;
    prof_data.instruction_counts[opcode]++;
    // pc is MEM_TWO_WORDS (uint16), so it's always < 65536
    prof_data.address_exec_counts[pc]++;
}

void profiler_dump_stats(void) {
    printf("\n[Profiler] Execution Statistics:\n");
    printf("  Total Cycles: %llu\n", prof_data.total_cycles);

    printf("\n  Instruction Frequency:\n");
    for (int i = 0; i < 256; i++) {
        if (prof_data.instruction_counts[i] > 0) {
            const INST* meta = &INST_TABLE[i >> 4][i & 0x0F];
            printf("    $%02X (%s): %llu times\n",
                   i, meta->CMD, prof_data.instruction_counts[i]);
        }
    }
}

void profiler_dump_hotspots(int top_n) {
    printf("\n[Profiler] Top %d Execution Hotspots:\n", top_n);

    // Create array of address/count pairs - allocate only what we need
    typedef struct {
        MEM_TWO_WORDS addr;
        unsigned long long count;
    } AddrCount;

    // First pass: count how many addresses were executed
    int active_count = 0;
    for (int addr = 0; addr < (int)PHY_MEM_SIZE; addr++) {
        if (prof_data.address_exec_counts[addr] > 0) {
            active_count++;
        }
    }

    if (active_count == 0) {
        printf("  No execution data recorded\n");
        return;
    }

    // Allocate only for active addresses
    AddrCount* hotspots = (AddrCount*)malloc(active_count * sizeof(AddrCount));
    if (!hotspots) {
        printf("  Error: Failed to allocate memory for hotspots\n");
        return;
    }

    int hotspot_count = 0;
    for (int addr = 0; addr < (int)PHY_MEM_SIZE; addr++) {
        if (prof_data.address_exec_counts[addr] > 0) {
            hotspots[hotspot_count].addr = (MEM_TWO_WORDS)addr;
            hotspots[hotspot_count].count = prof_data.address_exec_counts[addr];
            hotspot_count++;
        }
    }

    // Simple bubble sort for top_n (good enough for small n)
    for (int i = 0; i < hotspot_count && i < top_n; i++) {
        for (int j = i + 1; j < hotspot_count; j++) {
            if (hotspots[j].count > hotspots[i].count) {
                AddrCount temp = hotspots[i];
                hotspots[i] = hotspots[j];
                hotspots[j] = temp;
            }
        }
    }

    // Print top_n
    int limit = (top_n < hotspot_count) ? top_n : hotspot_count;
    for (int i = 0; i < limit; i++) {
        printf("  $%04X: %llu executions\n",
               hotspots[i].addr, hotspots[i].count);
    }

    free(hotspots);
}

void profiler_reset(void) {
    memset(&prof_data, 0, sizeof(prof_data));
    printf("[Profiler] Statistics reset\n");
}

PROFILING_DATA* profiler_get_data(void) {
    return &prof_data;
}


// Memory Inspection


void debugger_hexdump(MEM_TWO_WORDS start_addr, MEM_TWO_WORDS length) {
    printf("[Debugger] Memory Dump from $%04X (length: %u bytes):\n",
           start_addr, length);

    for (MEM_TWO_WORDS addr = start_addr; addr < start_addr + length; addr += 16) {
        printf("  %04X: ", addr);

        // Hex bytes
        for (int i = 0; i < 16; i++) {
            if (addr + i < start_addr + length) {
                printf("%02X ", bus_read(addr + i));
            } else {
                printf("   ");
            }
        }

        printf(" | ");

        // ASCII representation
        for (int i = 0; i < 16; i++) {
            if (addr + i < start_addr + length) {
                MEM_WORD byte = bus_read(addr + i);
                printf("%c", (byte >= 32 && byte < 127) ? byte : '.');
            }
        }

        printf("\n");
    }
}

void debugger_disassemble(MEM_TWO_WORDS start_addr, int num_instructions) {
    printf("[Debugger] Disassembly from $%04X:\n", start_addr);

    MEM_TWO_WORDS addr = start_addr;
    for (int i = 0; i < num_instructions; i++) {
        MEM_WORD opcode = bus_read(addr);
        const INST* meta = &INST_TABLE[opcode >> 4][opcode & 0x0F];

        printf("  %04X: %02X ", addr, opcode);

        // Print operand bytes
        int operand_size = 0;
        if (meta->ADDR == ADDR_IMM || meta->ADDR == ADDR_ZP ||
            meta->ADDR == ADDR_ZPX || meta->ADDR == ADDR_ZPY ||
            meta->ADDR == ADDR_INDX || meta->ADDR == ADDR_INDY ||
            meta->ADDR == ADDR_REL) {
            operand_size = 1;
        } else if (meta->ADDR == ADDR_ABS || meta->ADDR == ADDR_ABSX ||
                   meta->ADDR == ADDR_ABSY || meta->ADDR == ADDR_IND) {
            operand_size = 2;
        }

        for (int j = 0; j < operand_size; j++) {
            printf("%02X ", bus_read(addr + 1 + j));
        }
        for (int j = operand_size; j < 2; j++) {
            printf("   ");
        }

        printf(" %s\n", meta->CMD);

        addr += 1 + operand_size;
    }
}

void debugger_dump_registers(void) {
    printf("[Debugger] Registers:\n");
    printf("  PC: $%04X\n", REG.PC);
    printf("  A:  $%02X  X: $%02X  Y: $%02X\n", REG.A, REG.X, REG.Y);
    printf("  SP: $%02X  P: $%02X [", REG.S, REG.P);
    printf("%c", (REG.P & FLAG_N) ? 'N' : '-');
    printf("%c", (REG.P & FLAG_V) ? 'V' : '-');
    printf("%c", (REG.P & FLAG_U) ? 'U' : '-');
    printf("%c", (REG.P & FLAG_B) ? 'B' : '-');
    printf("%c", (REG.P & FLAG_D) ? 'D' : '-');
    printf("%c", (REG.P & FLAG_I) ? 'I' : '-');
    printf("%c", (REG.P & FLAG_Z) ? 'Z' : '-');
    printf("%c]\n", (REG.P & FLAG_C) ? 'C' : '-');
}

void debugger_dump_stack(void) {
    printf("[Debugger] Stack (SP=$%02X):\n", REG.S);
    MEM_TWO_WORDS stack_ptr = 0x0100 | REG.S;

    for (int i = 0; i < 16; i++) {
        MEM_TWO_WORDS addr = stack_ptr + i + 1;
        if (addr > 0x01FF) break;
        printf("  $%04X: $%02X\n", addr, bus_read(addr));
    }
}


// Interactive Debugger Control


void debugger_set_interactive(int enabled) {
    interactive_mode = enabled;
    if (enabled) {
        printf("[Debugger] Interactive mode enabled\n");
    }
}

int debugger_is_interactive(void) {
    return interactive_mode;
}

void debugger_step(void) {
    // This will be called from cpu_step()
    debugger_paused = 0;
}

void debugger_continue(void) {
    debugger_paused = 0;
}


// Memory Search


int debugger_search_memory(MEM_TWO_WORDS start, MEM_TWO_WORDS end,
                           const MEM_WORD* pattern, MEM_TWO_WORDS pattern_len) {
    printf("[Debugger] Searching memory $%04X-$%04X for pattern...\n", start, end);
    int found_count = 0;

    for (MEM_TWO_WORDS addr = start; addr <= end - pattern_len; addr++) {
        int match = 1;
        for (MEM_TWO_WORDS i = 0; i < pattern_len; i++) {
            if (bus_read(addr + i) != pattern[i]) {
                match = 0;
                break;
            }
        }

        if (match) {
            printf("  Found at $%04X\n", addr);
            found_count++;
        }
    }

    printf("[Debugger] Found %d match(es)\n", found_count);
    return found_count;
}
