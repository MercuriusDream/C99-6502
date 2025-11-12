#ifndef DISPLAY_H
#define DISPLAY_H

#include "types.h"

// Initialize SDL display
int display_init(void);

// Shutdown SDL display
void display_cleanup(void);

// Refresh display from Apple II memory
void display_refresh(void);

// Handle SDL events (keyboard, quit, etc.)
// Returns 0 to continue, 1 to quit
int display_handle_events(void);

// Get current keyboard state for Apple II ($C000)
MEM_WORD display_get_keyboard(void);

// Clear keyboard strobe ($C010)
void display_clear_keyboard_strobe(void);

#endif
