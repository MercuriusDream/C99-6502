#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "types.h"

// Maximum number of breakpoints and watchpoints
#define MAX_BREAKPOINTS 32
#define MAX_WATCHPOINTS 16

// Breakpoint types
typedef enum {
    BP_TYPE_EXEC = 0,      // Execution breakpoint
    BP_TYPE_READ,          // Memory read breakpoint
    BP_TYPE_WRITE,         // Memory write breakpoint
    BP_TYPE_ACCESS         // Memory read or write breakpoint
} BP_TYPE;

// Breakpoint structure
typedef struct {
    int enabled;
    BP_TYPE type;
    MEM_TWO_WORDS addr;
    const char* label;     // Optional label for the breakpoint
} BREAKPOINT;

// Watchpoint structure
typedef struct {
    int enabled;
    MEM_TWO_WORDS addr;
    MEM_WORD old_value;
    const char* label;
} WATCHPOINT;

// Profiling data structure
typedef struct {
    unsigned long long total_cycles;
    unsigned long long instruction_counts[256];
    unsigned long long address_exec_counts[PHY_MEM_SIZE];
} PROFILING_DATA;

// Debugger initialization and cleanup
void debugger_init(void);
void debugger_cleanup(void);

// Breakpoint management
int debugger_add_breakpoint(BP_TYPE type, MEM_TWO_WORDS addr, const char* label);
int debugger_remove_breakpoint(int index);
int debugger_enable_breakpoint(int index, int enabled);
int debugger_check_breakpoint(BP_TYPE type, MEM_TWO_WORDS addr);
void debugger_list_breakpoints(void);
void debugger_clear_breakpoints(void);

// Watchpoint management
int debugger_add_watchpoint(MEM_TWO_WORDS addr, const char* label);
int debugger_remove_watchpoint(int index);
int debugger_enable_watchpoint(int index, int enabled);
void debugger_check_watchpoints(void);
void debugger_list_watchpoints(void);
void debugger_clear_watchpoints(void);

// Profiling functions
void profiler_init(void);
void profiler_record_instruction(MEM_TWO_WORDS pc, MEM_WORD opcode, unsigned int cycles);
void profiler_dump_stats(void);
void profiler_dump_hotspots(int top_n);
void profiler_reset(void);
PROFILING_DATA* profiler_get_data(void);

// Memory inspection
void debugger_hexdump(MEM_TWO_WORDS start_addr, MEM_TWO_WORDS length);
void debugger_disassemble(MEM_TWO_WORDS start_addr, int num_instructions);
void debugger_dump_registers(void);
void debugger_dump_stack(void);

// Interactive debugger control
void debugger_set_interactive(int enabled);
int debugger_is_interactive(void);
void debugger_step(void);  // Single step execution
void debugger_continue(void);  // Continue until breakpoint

// Memory search
int debugger_search_memory(MEM_TWO_WORDS start, MEM_TWO_WORDS end,
                           const MEM_WORD* pattern, MEM_TWO_WORDS pattern_len);

#endif
