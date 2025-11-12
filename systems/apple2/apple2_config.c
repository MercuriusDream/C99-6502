#include "system_config.h"
#include "memory.h"
#include "bus.h"
#include "loader.h"
#include "disk_image.h"
#include "disk_controller.h"
#include "display.h"
#include <stdio.h>

static DISK_IMAGE apple2_disk;

static MEM_WORD apple2_kb_read(MEM_TWO_WORDS addr, void* ctx) {
    (void)ctx;
    if (addr == 0xC000) {
        // Read keyboard data (does NOT clear strobe)
        MEM_WORD key = display_get_keyboard();
        if (key & 0x80) {
            // Only print when there's actually a key ready (for debugging)
            static int last_logged = 0;
            if (key != last_logged) {
                printf("[KB READ] $C000 read, returning $%02X\n", key);
                last_logged = key;
            }
        }
        return key;
    } else if (addr == 0xC010) {
        // Reading $C010 clears keyboard strobe and returns previous key
        MEM_WORD prev_key = display_get_keyboard();
        printf("[KB STROBE] $C010 accessed, clearing strobe (key was $%02X)\n", prev_key);
        display_clear_keyboard_strobe();
        return prev_key;
    }
    return 0x00;
}

static void apple2_kb_write(MEM_TWO_WORDS addr, MEM_WORD data, void* ctx) {
    (void)data;
    (void)ctx;
    if (addr == 0xC010) {
        // Writing to $C010 also clears keyboard strobe
        display_clear_keyboard_strobe();
    }
}

static void apple2_clear_screen(void) {
    // Clear text screen with spaces ($A0 = space in Apple II encoding)
    for (int i = 0x0400; i < 0x0800; i++) {
        bus_write(i, 0xA0);
    }

    // Clear hi-res screen
    for (int i = 0x2000; i < 0x4000; i++) {
        bus_write(i, 0x00);
    }
}

static void apple2_setup_boot(void) {
    // NEW APPROACH: Boot through Disk II ROM like a real Apple II
    // The Disk II ROM will:
    // 1. Read the boot sector from disk using our disk controller
    // 2. Load it to $0800
    // 3. Jump to $0801 to execute it

    if (apple2_disk.disk_data == NULL) {
        printf("[BOOT] No disk mounted\n");
        return;
    }

    printf("\n[BOOT] Disk II ROM will load boot sector from disk\n");
    printf("[BOOT] Disk image: %u blocks (%u KB)\n",
           apple2_disk.disk_size / 512, apple2_disk.disk_size / 1024);
    printf("[BOOT] First sector bytes: %02X %02X %02X %02X\n",
           apple2_disk.disk_data[0], apple2_disk.disk_data[1],
           apple2_disk.disk_data[2], apple2_disk.disk_data[3]);
}

static void apple2_init_peripherals(void) {
    printf("Initializing Apple II peripherals...\n");

    // Initialize SDL display
    if (display_init() != 0) {
        printf("Warning: Failed to initialize display\n");
    } else {
        printf("Display: SDL (text + hi-res)\n");
    }

    printf("Keyboard: $C000\n");
    printf("Text screen: $0400-$07FF\n");
    printf("Disk II: $C0E0-$C0EF (slot 6)\n");
    printf("Disk II ROM: $C600-$C6FF\n");

    // Keyboard I/O ($C000 = read, $C010 = strobe clear)
    mem_region_add_io(0xC000, 0x1, apple2_kb_read, apple2_kb_write, NULL);
    mem_region_add_io(0xC010, 0x1, apple2_kb_read, apple2_kb_write, NULL);

    // Disk II controller I/O
    mem_region_add_io(0xC0E0, 0x10, disk_controller_read, disk_controller_write, NULL);

    // Disk II ROM
    mem_region_add_rom(0xC600, 0x100);
    if (load_bin_region("dsk2.bin", 0xC600) != 0) {
        printf("Warning: Failed to load Disk II ROM (dsk2.bin)\n");
    }

    // Apple II ROM - but we need to patch it, so load as RAM first
    mem_region_add_ram(0xD000, 0x3000);
    if (load_bin_region("apple2o.rom", 0xD000) != 0) {
        printf("Warning: Failed to load Apple II ROM (apple2o.rom)\n");
    }

    // TEST MODE: Leave reset vector pointing to Monitor ROM
    // The Apple II ROM's reset vector naturally points to Monitor at $FA62
    // We'll verify what it points to
    MEM_WORD lo = bus_read(0xFFFC);
    MEM_WORD hi = bus_read(0xFFFD);
    printf("Reset vector: $%02X%02X (Apple II Monitor ROM)\n", hi, lo);

    // Try to mount disk image
    if (disk_image_open(&apple2_disk, "pop.po") == 0) {
        printf("\nMounted disk: pop.po\n");
        disk_controller_init(&apple2_disk);
    } else {
        printf("\nNo disk image mounted (pop.po not found)\n");
    }
}

static void apple2_post_init(void) {
    // Clear screen
    apple2_clear_screen();

    // Print boot info (Disk II ROM will handle actual loading)
    apple2_setup_boot();
}

static const SYSTEM_CONFIG apple2_config = {
    .name = "Apple II",
    .cpu_variant = CPU_VARIANT_NMOS_6502,
    .ram_start = 0x0000,
    .ram_size = 48 * 1024,
    .rom_start = 0xD000,
    .rom_size = 12 * 1024,
    .reset_vector = 0xC600,  // Boot through Disk II ROM (proper boot sequence)
    .init_peripherals = apple2_init_peripherals,
    .post_init = apple2_post_init
};

const SYSTEM_CONFIG* apple2_get_config(void) {
    return &apple2_config;
}
