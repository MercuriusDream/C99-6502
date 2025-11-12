#ifndef INTERACTIVE_DEBUGGER_H
#define INTERACTIVE_DEBUGGER_H

#include "types.h"

typedef enum {
    IDBG_MODE_RUN,
    IDBG_MODE_STEP,
    IDBG_MODE_QUIT
} IDBG_MODE;

void idbg_init(void);
void idbg_start(void);
int idbg_should_pause(void);
void idbg_handle_breakpoint(MEM_TWO_WORDS pc);
IDBG_MODE idbg_get_mode(void);
void idbg_set_mode(IDBG_MODE mode);
int idbg_process_command(const char* command);
void idbg_show_context(void);

#endif
