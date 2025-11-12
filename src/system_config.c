#include "system_config.h"
#include <stdio.h>
#include <string.h>

const SYSTEM_CONFIG* system_get_config(const char* system_name) {
    if (strcmp(system_name, "apple2") == 0 || strcmp(system_name, "appleii") == 0) {
        return apple2_get_config();
    } else if (strcmp(system_name, "nes") == 0) {
        return nes_get_config();
    } else if (strcmp(system_name, "c64") == 0 || strcmp(system_name, "commodore64") == 0) {
        return c64_get_config();
    }
    return NULL;
}

void system_list_configs(void) {
    printf("Available system configurations:\n");
    printf("  apple2     - Apple II (6502, 48KB RAM, 16KB ROM)\n");
    printf("  nes        - Nintendo Entertainment System (6502, 2KB RAM, 32KB ROM)\n");
    printf("  c64        - Commodore 64 (6502, 64KB RAM, 8KB BASIC ROM)\n");
}
