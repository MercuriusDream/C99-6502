#include "memory.h"
#include "bus.h"
#include "types.h"
#include <stdlib.h>
#include <string.h>

// Region-based memory system
static MEM_REGION REGIONS[MAX_MEM_REGIONS];
static int REGION_COUNT = 0;

void mem_region_clear() {
    for (int i = 0; i < REGION_COUNT; i++) {
        if (REGIONS[i].DATA) {
            free(REGIONS[i].DATA);
            REGIONS[i].DATA = NULL;
        }
    }
    REGION_COUNT = 0;
}

int mem_region_add_ram(MEM_TWO_WORDS START, MEM_TWO_WORDS SIZE) {
    if (REGION_COUNT >= MAX_MEM_REGIONS) return -1;

    MEM_REGION* region = &REGIONS[REGION_COUNT++];
    region->START = START;
    region->END = START + SIZE - 1;
    region->TYPE = MEM_REGION_RAM;
    region->DATA = (MEM_WORD*)calloc(SIZE, sizeof(MEM_WORD));
    region->READ_HANDLER = NULL;
    region->WRITE_HANDLER = NULL;
    region->CTX = NULL;

    return 0;
}

int mem_region_add_rom(MEM_TWO_WORDS START, MEM_TWO_WORDS SIZE) {
    if (REGION_COUNT >= MAX_MEM_REGIONS) return -1;

    MEM_REGION* region = &REGIONS[REGION_COUNT++];
    region->START = START;
    region->END = START + SIZE - 1;
    region->TYPE = MEM_REGION_ROM;
    region->DATA = (MEM_WORD*)calloc(SIZE, sizeof(MEM_WORD));
    region->READ_HANDLER = NULL;
    region->WRITE_HANDLER = NULL;
    region->CTX = NULL;

    return 0;
}

int mem_region_add_io(MEM_TWO_WORDS START, MEM_TWO_WORDS SIZE,
                      bus_read_fn READ_HANDLER, bus_write_fn WRITE_HANDLER, void* CTX) {
    if (REGION_COUNT >= MAX_MEM_REGIONS) return -1;

    MEM_REGION* region = &REGIONS[REGION_COUNT++];
    region->START = START;
    region->END = START + SIZE - 1;
    region->TYPE = MEM_REGION_IO;
    region->DATA = NULL;
    region->READ_HANDLER = READ_HANDLER;
    region->WRITE_HANDLER = WRITE_HANDLER;
    region->CTX = CTX;

    return 0;
}

static MEM_REGION* find_region(MEM_TWO_WORDS ADDR) {
    for (int i = 0; i < REGION_COUNT; i++) {
        if (ADDR >= REGIONS[i].START && ADDR <= REGIONS[i].END) {
            return &REGIONS[i];
        }
    }
    return NULL;
}

static MEM_WORD region_bus_read(MEM_TWO_WORDS ADDR, void* CTX) {
    (void)CTX;  // Unused

    MEM_REGION* region = find_region(ADDR);
    if (!region) return 0xFF;  // Open bus behavior

    // I/O region with custom handler
    if (region->TYPE == MEM_REGION_IO && region->READ_HANDLER) {
        return region->READ_HANDLER(ADDR, region->CTX);
    }

    // RAM or ROM region
    if (region->DATA) {
        MEM_TWO_WORDS offset = ADDR - region->START;
        return region->DATA[offset];
    }

    return 0xFF;
}

static void region_bus_write(MEM_TWO_WORDS ADDR, MEM_WORD DATA, void* CTX) {
    (void)CTX;  // Unused

    MEM_REGION* region = find_region(ADDR);
    if (!region) return;  // Open bus, ignore write

    // ROM region - ignore writes (write protection)
    if (region->TYPE == MEM_REGION_ROM) {
        return;
    }

    // I/O region with custom handler
    if (region->TYPE == MEM_REGION_IO && region->WRITE_HANDLER) {
        region->WRITE_HANDLER(ADDR, DATA, region->CTX);
        return;
    }

    // RAM region
    if (region->TYPE == MEM_REGION_RAM && region->DATA) {
        MEM_TWO_WORDS offset = ADDR - region->START;
        region->DATA[offset] = DATA;
    }
}

void mem_region_init() {
    BUS.CTX = NULL;
    BUS.READ = region_bus_read;
    BUS.WRITE = region_bus_write;
}

// Helper functions

int mem_region_load(MEM_TWO_WORDS ADDR, const MEM_WORD* DATA, MEM_TWO_WORDS LEN) {
    for (MEM_TWO_WORDS i = 0; i < LEN; i++) {
        MEM_REGION* region = find_region(ADDR + i);
        if (!region || !region->DATA) return -1;

        MEM_TWO_WORDS offset = (ADDR + i) - region->START;
        region->DATA[offset] = DATA[i];
    }
    return 0;
}

int mem_region_set_vector(MEM_TWO_WORDS VEC, MEM_TWO_WORDS DEST) {
    MEM_REGION* region = find_region(VEC);
    if (!region || !region->DATA) return -1;

    MEM_TWO_WORDS offset = VEC - region->START;
    region->DATA[offset] = (MEM_WORD)(DEST & 0xFF);
    region->DATA[offset + 1] = (MEM_WORD)((DEST >> 8) & 0xFF);
    return 0;
}

MEM_WORD* mem_region_get_ptr(MEM_TWO_WORDS ADDR) {
    MEM_REGION* region = find_region(ADDR);
    if (!region || !region->DATA) return NULL;

    MEM_TWO_WORDS offset = ADDR - region->START;
    return &region->DATA[offset];
}
