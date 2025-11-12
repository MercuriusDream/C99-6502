#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "types.h"

typedef struct {
    const char* name;
    CPU_VARIANT cpu_variant;
    MEM_TWO_WORDS ram_start;
    MEM_TWO_WORDS ram_size;
    MEM_TWO_WORDS rom_start;
    MEM_TWO_WORDS rom_size;
    MEM_TWO_WORDS reset_vector;
    void (*init_peripherals)(void);
    void (*post_init)(void);  // Called after memory bus is connected
} SYSTEM_CONFIG;

void system_load_config(const char* system_name);
const SYSTEM_CONFIG* system_get_config(const char* system_name);
void system_list_configs(void);

const SYSTEM_CONFIG* apple2_get_config(void);
const SYSTEM_CONFIG* nes_get_config(void);
const SYSTEM_CONFIG* c64_get_config(void);

#endif
