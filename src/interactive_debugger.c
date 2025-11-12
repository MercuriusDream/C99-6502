#define _POSIX_C_SOURCE 200809L
#include "interactive_debugger.h"
#include "debugger.h"
#include "cpu.h"
#include "bus.h"
#include "instruments_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CMD_LEN 256
#define MAX_TOKENS 8


// State


static IDBG_MODE current_mode = IDBG_MODE_RUN;
static int step_count = 0;


// Helpers


static void trim(char* str) {
    char* end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

static int tokenize(char* str, char** tokens, int max_tokens) {
    int count = 0;
    char* token = strtok(str, " \t");
    while (token != NULL && count < max_tokens) {
        tokens[count++] = token;
        token = strtok(NULL, " \t");
    }
    return count;
}

static MEM_TWO_WORDS parse_addr(const char* str) {
    if (str[0] == '$') {
        return (MEM_TWO_WORDS)strtol(str + 1, NULL, 16);
    }
    return (MEM_TWO_WORDS)strtol(str, NULL, 16);
}


// Command Handlers


static void cmd_help(void) {
    printf("\nDebugger Commands:\n");
    printf("  h, help              Show this help\n");
    printf("  s, step [n]          Step one or n instructions\n");
    printf("  c, continue          Continue execution\n");
    printf("  r, registers         Show CPU registers\n");
    printf("  d, disasm [addr] [n] Disassemble n instructions at addr\n");
    printf("  x, examine [addr] [n] Examine n bytes at addr\n");
    printf("  b, break <addr>      Set execution breakpoint\n");
    printf("  w, watch <addr>      Set watchpoint\n");
    printf("  lb, list-breaks      List breakpoints\n");
    printf("  lw, list-watches     List watchpoints\n");
    printf("  cb, clear-breaks     Clear all breakpoints\n");
    printf("  cw, clear-watches    Clear all watchpoints\n");
    printf("  stack                Show stack\n");
    printf("  search <addr> <byte>... Search for byte pattern\n");
    printf("  reset                Reset CPU\n");
    printf("  q, quit              Quit emulator\n");
    printf("\n");
}

static void cmd_step(int count) {
    step_count = count;
    current_mode = IDBG_MODE_STEP;
}

static void cmd_continue(void) {
    current_mode = IDBG_MODE_RUN;
    printf("Continuing...\n");
}

static void cmd_disasm(int argc, char** argv) {
    MEM_TWO_WORDS addr = REG.PC;
    int count = 10;

    if (argc >= 2) addr = parse_addr(argv[1]);
    if (argc >= 3) count = atoi(argv[2]);

    debugger_disassemble(addr, count);
}

static void cmd_examine(int argc, char** argv) {
    MEM_TWO_WORDS addr = 0x0000;
    int count = 64;

    if (argc >= 2) addr = parse_addr(argv[1]);
    if (argc >= 3) count = atoi(argv[2]);

    debugger_hexdump(addr, count);
}

static void cmd_break(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: break <addr>\n");
        return;
    }
    MEM_TWO_WORDS addr = parse_addr(argv[1]);
    debugger_add_breakpoint(BP_TYPE_EXEC, addr, NULL);
}

static void cmd_watch(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: watch <addr>\n");
        return;
    }
    MEM_TWO_WORDS addr = parse_addr(argv[1]);
    debugger_add_watchpoint(addr, NULL);
}

static void cmd_search(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: search <addr> <byte> [byte...]\n");
        return;
    }

    MEM_TWO_WORDS start_addr = parse_addr(argv[1]);
    MEM_WORD pattern[16];
    int pattern_len = 0;

    for (int i = 2; i < argc && pattern_len < 16; i++) {
        pattern[pattern_len++] = (MEM_WORD)strtol(argv[i], NULL, 16);
    }

    debugger_search_memory(start_addr, 0xFFFF, pattern, pattern_len);
}


// Public Interface


void idbg_init(void) {
    current_mode = IDBG_MODE_RUN;
    step_count = 0;
    debugger_set_interactive(1);
}

void idbg_start(void) {
    char input[MAX_CMD_LEN];

    printf("\nInteractive Debugger\n");
    printf("Type 'help' for commands\n\n");

    idbg_show_context();

    while (1) {
        printf("(6502) ");
        fflush(stdout);

        if (!fgets(input, MAX_CMD_LEN, stdin)) {
            break;
        }

        trim(input);
        if (strlen(input) == 0) {
            continue;
        }

        if (idbg_process_command(input)) {
            break;
        }
    }
}

int idbg_should_pause(void) {
    if (current_mode == IDBG_MODE_STEP) {
        if (step_count > 0) {
            step_count--;
            return 0;
        }
        return 1;
    }
    return 0;
}

void idbg_handle_breakpoint(MEM_TWO_WORDS pc) {
    printf("\nBreakpoint hit at $%04X\n", pc);
    idbg_show_context();
    current_mode = IDBG_MODE_STEP;
    step_count = 0;
}

IDBG_MODE idbg_get_mode(void) {
    return current_mode;
}

void idbg_set_mode(IDBG_MODE mode) {
    current_mode = mode;
}

int idbg_process_command(const char* command) {
    char cmd_copy[MAX_CMD_LEN];
    char* tokens[MAX_TOKENS];

    strncpy(cmd_copy, command, MAX_CMD_LEN - 1);
    cmd_copy[MAX_CMD_LEN - 1] = '\0';

    int argc = tokenize(cmd_copy, tokens, MAX_TOKENS);
    if (argc == 0) return 0;

    const char* cmd = tokens[0];

    if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "s") == 0 || strcmp(cmd, "step") == 0) {
        int count = (argc >= 2) ? atoi(tokens[1]) : 1;
        cmd_step(count);
        return 1;
    } else if (strcmp(cmd, "c") == 0 || strcmp(cmd, "continue") == 0) {
        cmd_continue();
        return 1;
    } else if (strcmp(cmd, "r") == 0 || strcmp(cmd, "registers") == 0) {
        debugger_dump_registers();
    } else if (strcmp(cmd, "d") == 0 || strcmp(cmd, "disasm") == 0) {
        cmd_disasm(argc, tokens);
    } else if (strcmp(cmd, "x") == 0 || strcmp(cmd, "examine") == 0) {
        cmd_examine(argc, tokens);
    } else if (strcmp(cmd, "b") == 0 || strcmp(cmd, "break") == 0) {
        cmd_break(argc, tokens);
    } else if (strcmp(cmd, "w") == 0 || strcmp(cmd, "watch") == 0) {
        cmd_watch(argc, tokens);
    } else if (strcmp(cmd, "lb") == 0 || strcmp(cmd, "list-breaks") == 0) {
        debugger_list_breakpoints();
    } else if (strcmp(cmd, "lw") == 0 || strcmp(cmd, "list-watches") == 0) {
        debugger_list_watchpoints();
    } else if (strcmp(cmd, "cb") == 0 || strcmp(cmd, "clear-breaks") == 0) {
        debugger_clear_breakpoints();
    } else if (strcmp(cmd, "cw") == 0 || strcmp(cmd, "clear-watches") == 0) {
        debugger_clear_watchpoints();
    } else if (strcmp(cmd, "stack") == 0) {
        debugger_dump_stack();
    } else if (strcmp(cmd, "search") == 0) {
        cmd_search(argc, tokens);
    } else if (strcmp(cmd, "reset") == 0) {
        cpu_reset();
        printf("CPU reset\n");
        idbg_show_context();
    } else if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
        current_mode = IDBG_MODE_QUIT;
        return 1;
    } else {
        printf("Unknown command: %s (type 'help' for commands)\n", cmd);
    }

    return 0;
}

void idbg_show_context(void) {
    debugger_dump_registers();
    printf("\n");
    debugger_disassemble(REG.PC, 5);
}
