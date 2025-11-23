#include "loader.h"
#include "memory.h"
#include <stdio.h>

int load_bin_region(const char* PATH, MEM_TWO_WORDS ADDR) {
    FILE* f = fopen(PATH, "rb");
    if (!f) return -1;

    unsigned char buf[4096];
    size_t n;
    MEM_TWO_WORDS off = 0;

    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (mem_region_load(ADDR + off, buf, (MEM_TWO_WORDS)n) != 0) {
            fclose(f);
            return -1;
        }
        off += (MEM_TWO_WORDS)n;
    }

    // Check for read errors (not just EOF)
    if (ferror(f)) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}
