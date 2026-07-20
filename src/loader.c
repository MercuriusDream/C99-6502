#include "loader.h"
#include "memory.h"
#include <stdio.h>
#include <stdint.h>

int load_bin_region(const char* PATH, MEM_TWO_WORDS ADDR) {
    FILE* f = fopen(PATH, "rb");
    if (!f) return -1;

    // Determine file size before reading.
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long file_size_l = ftell(f);
    if (file_size_l < 0) {
        fclose(f);
        return -1;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }

    uint32_t file_size = (uint32_t)file_size_l;

    // Reject if the file would cross the 64KiB address-space boundary.
    if ((uint32_t)ADDR + file_size > 0x10000) {
        fclose(f);
        return -1;
    }

    unsigned char buf[4096];
    size_t n;
    uint32_t off = 0;

    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (mem_region_load((MEM_TWO_WORDS)(ADDR + off), buf, (MEM_TWO_WORDS)n) != 0) {
            fclose(f);
            return -1;
        }
        off += (uint32_t)n;
    }

    if (ferror(f)) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}