#ifndef DISK_CONTROLLER_H
#define DISK_CONTROLLER_H

#include "types.h"
#include "disk_image.h"

typedef struct {
    DISK_IMAGE* disk_image;

    // Drive state
    int motor_on;
    int drive_selected;  // 0 or 1
    int current_track;
    int current_sector;

    // Phase state (stepper motor)
    int phases[4];

    // Q6/Q7 state
    int q6;  // 0 = read, 1 = sense write protect
    int q7;  // 0 = read mode, 1 = write mode

    // Read state
    int read_mode;
    int byte_offset;
    MEM_WORD sector_buffer[256];     // Raw sector data (256 bytes per sector)
    MEM_WORD track_nibbles[8192];    // Full track nibbles (allow generous gap space)
    int track_size;                  // Size of nibblized track
    int track_loaded;                // Track loaded flag
} DISK_CONTROLLER;

// Initialize disk controller
void disk_controller_init(DISK_IMAGE* img);

// Cleanup
void disk_controller_cleanup(void);

// I/O handlers for $C0E0-$C0EF
MEM_WORD disk_controller_read(MEM_TWO_WORDS addr, void* ctx);
void disk_controller_write(MEM_TWO_WORDS addr, MEM_WORD data, void* ctx);

#endif
