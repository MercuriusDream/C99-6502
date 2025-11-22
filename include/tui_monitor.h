#ifndef TUI_MONITOR_H
#define TUI_MONITOR_H

#include <ncurses.h>
#include <time.h>
#include "types.h"

// Panel dimensions and positions
#define SCREEN_MIN_WIDTH 85
#define SCREEN_MIN_HEIGHT 19

// Cycle history configuration - larger sample for better percentile calculation
#define CYCLE_HISTORY_SIZE 300
#define CYCLE_HISTORY_HEIGHT 4

// Instruction log configuration
#define INSTRUCTION_LOG_SIZE 100
#define INSTRUCTION_LOG_DISPLAY 10

// Memory watch configuration
#define MEMORY_WATCH_SIZE 8

// Memory watch entry with change tracking
typedef struct {
    MEM_TWO_WORDS addr;
    MEM_WORD value;
    MEM_WORD prev_value;
    int changed;
} MemoryWatch;

// Instruction log entry
typedef struct {
    uint64_t cycle;
    MEM_TWO_WORDS addr;
    MEM_WORD opcode;
    char mnemonic[16];
    char state_change[32];
    struct timespec timestamp;
} InstructionLogEntry;

// Bus activity state
typedef enum {
    BUS_IDLE,
    BUS_READ,
    BUS_WRITE
} BusState;

typedef struct {
    BusState state;
    MEM_TWO_WORDS addr;
    MEM_WORD data;
    char target[16];
    MEM_TWO_WORDS last_write_addr;
    MEM_WORD last_write_data;
    MEM_TWO_WORDS last_read_addr;
    MEM_WORD last_read_data;
} BusActivity;

// System configuration info
typedef struct {
    MEM_TWO_WORDS ram_start;
    MEM_TWO_WORDS ram_size;
    MEM_TWO_WORDS rom_start;
    MEM_TWO_WORDS rom_size;
    CPU_VARIANT cpu_variant;
} SystemConfig;

// Monitor state
typedef struct {
    // Execution state
    int running;
    int stepping;
    int should_quit;

    // Timing
    struct timespec start_time;
    uint64_t total_cycles;
    double current_mhz;

    // Cycle history for graph
    uint64_t cycle_history[CYCLE_HISTORY_SIZE];
    int cycle_history_idx;

    // Instruction log
    InstructionLogEntry instruction_log[INSTRUCTION_LOG_SIZE];
    int instruction_log_idx;
    int instruction_log_count;

    // Bus activity
    BusActivity bus;

    // Memory watches
    MemoryWatch watches[MEMORY_WATCH_SIZE];
    int watch_count;

    // Disassembly view
    MEM_TWO_WORDS disasm_addr;

    // Interrupt indicators
    int irq_pending;
    int nmi_pending;
    int reset_pending;

    // System configuration
    SystemConfig sys_config;
} MonitorState;

// TUI Functions
void tui_init(void);
void tui_cleanup(void);
void tui_draw(MonitorState* state);
void tui_handle_input(MonitorState* state);

// Panel drawing functions
void draw_cpu_status(MonitorState* state, int y, int x, int width, int height);
void draw_system_info(MonitorState* state, int y, int x, int width, int height);
void draw_execution_history(MonitorState* state, int y, int x, int width, int height);
void draw_bus_activity(MonitorState* state, int y, int x, int width, int height);
void draw_instruction_log(MonitorState* state, int y, int x, int width, int height);
void draw_registers(MonitorState* state, int y, int x, int width, int height);
void draw_stack_depth(MonitorState* state, int y, int x, int width, int height);
void draw_disassembly(MonitorState* state, int y, int x, int width, int height);
void draw_memory_watch(MonitorState* state, int y, int x, int width, int height);
void draw_help_bar(int y, int x, int width);

// Monitor state functions
void monitor_state_init(MonitorState* state);
void monitor_state_update(MonitorState* state);
void monitor_log_instruction(MonitorState* state, MEM_TWO_WORDS pc, MEM_WORD opcode);
void monitor_update_bus(MonitorState* state, BusState bus_state, MEM_TWO_WORDS addr, MEM_WORD data);
void monitor_add_watch(MonitorState* state, MEM_TWO_WORDS addr);

// Utility functions
void format_uptime(struct timespec start, char* buf, size_t bufsize);
double calculate_mhz(uint64_t cycles, struct timespec start);

#endif // TUI_MONITOR_H
