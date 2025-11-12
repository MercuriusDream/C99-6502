#include "disk_controller.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

static DISK_CONTROLLER disk_ctrl;

// 6-and-2 nibble encoding tables
// These translate 6-bit values (0-63) to valid disk bytes
static const MEM_WORD nibble_6to2[64] = {
    0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6,
    0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
    0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC,
    0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3,
    0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE,
    0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
    0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
    0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

// 4-and-4 encoding for address fields
// Encodes a byte as two nibbles with high bit set
// Example: value 0x00 -> returns two bytes: 0xAA 0xAA
// Example: value 0xFF -> returns two bytes: 0xAF 0xBF (encoded as odd/even)
static void encode_4and4_pair(MEM_WORD value, MEM_WORD* out) {
    // Encode as odd-even format with bit 7 set
    // Odd bits in first byte, even bits in second byte
    out[0] = ((value >> 1) & 0x55) | 0xAA;  // Odd bits
    out[1] = (value & 0x55) | 0xAA;          // Even bits
}

// DOS 3.3 physical sector interleave
// Physical sectors appear on disk in this logical order
static const int dos33_interleave[16] = {
    0, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 15
};

void disk_controller_init(DISK_IMAGE* img) {
    memset(&disk_ctrl, 0, sizeof(disk_ctrl));
    disk_ctrl.disk_image = img;
    disk_ctrl.motor_on = 0;
    disk_ctrl.drive_selected = 0;
    disk_ctrl.current_track = 0;
    disk_ctrl.current_sector = 0;
    disk_ctrl.q6 = 0;
    disk_ctrl.q7 = 0;
    disk_ctrl.read_mode = 1;
    disk_ctrl.byte_offset = 0;
    disk_ctrl.track_loaded = 0;

    printf("Disk II controller initialized\n");
    printf("  Disk size: %u blocks\n", img->disk_size / 512);
}

void disk_controller_cleanup(void) {
    memset(&disk_ctrl, 0, sizeof(disk_ctrl));
}

// Nibblize a 256-byte sector into disk format
// Returns size of nibblized data
static int nibblize_sector(const MEM_WORD* raw_data, int track, int sector, int volume, MEM_WORD* out) {
    int pos = 0;

    // Work buffers for canonical 6-and-2 encoding
    MEM_WORD data_copy[256];
    memcpy(data_copy, raw_data, sizeof(data_copy));

    MEM_WORD aux_bytes[86];
    memset(aux_bytes, 0, sizeof(aux_bytes));

    // Self-sync bytes (gap) - need more for proper sync
    // Real Apple II disks had 10-128 self-sync bytes before address field
    for (int i = 0; i < 32; i++) {
        out[pos++] = 0xFF;
    }

    // Address field prologue
    out[pos++] = 0xD5;
    out[pos++] = 0xAA;
    out[pos++] = 0x96;

    // Volume, track, sector, checksum (4-and-4 encoded - 2 bytes each)
    MEM_WORD encoded[2];
    encode_4and4_pair(volume, encoded);
    out[pos++] = encoded[0];
    out[pos++] = encoded[1];

    encode_4and4_pair(track, encoded);
    out[pos++] = encoded[0];
    out[pos++] = encoded[1];

    encode_4and4_pair(sector, encoded);
    out[pos++] = encoded[0];
    out[pos++] = encoded[1];

    encode_4and4_pair(volume ^ track ^ sector, encoded);
    out[pos++] = encoded[0];
    out[pos++] = encoded[1];

    // Address field epilogue
    out[pos++] = 0xDE;
    out[pos++] = 0xAA;
    out[pos++] = 0xEB;

    // Gap before data field - more sync bytes
    for (int i = 0; i < 10; i++) {
        out[pos++] = 0xFF;
    }

    // Data field prologue
    out[pos++] = 0xD5;
    out[pos++] = 0xAA;
    out[pos++] = 0xAD;

    // Perform real 6-and-2 packing (Beneath Apple DOS algorithm)
    int aux_index = 0;
    for (int i = 0; i < 256; i++) {
        MEM_WORD val = data_copy[i];

        // Append low bit then next bit to rotating aux buffer entry
        aux_bytes[aux_index] = ((aux_bytes[aux_index] << 1) | (val & 0x01)) & 0x3F;
        val >>= 1;
        aux_bytes[aux_index] = ((aux_bytes[aux_index] << 1) | (val & 0x01)) & 0x3F;
        val >>= 1;

        data_copy[i] = val & 0x3F;  // Remaining upper 6 bits

        aux_index++;
        if (aux_index == 86) {
            aux_index = 0;
        }
    }

    MEM_WORD buffer[342];
    int buf_pos = 0;

    for (int i = 0; i < 86; i++) {
        buffer[buf_pos++] = aux_bytes[i];
    }
    for (int i = 0; i < 256; i++) {
        buffer[buf_pos++] = data_copy[i];
    }

    // Step 3: XOR encode with running checksum and translate through nibble table
    MEM_WORD checksum = 0;
    for (int i = 0; i < buf_pos; i++) {
        checksum ^= buffer[i];
        out[pos++] = nibble_6to2[checksum & 0x3F];
    }

    // Step 4: Final checksum byte
    out[pos++] = nibble_6to2[checksum & 0x3F];

    // Data field epilogue
    out[pos++] = 0xDE;
    out[pos++] = 0xAA;
    out[pos++] = 0xEB;

    // Trailing gap
    for (int i = 0; i < 16; i++) {
        out[pos++] = 0xFF;
    }

    return pos;
}

// Load and nibblize an entire track (all 16 sectors)
static void load_track(int track) {
    int pos = 0;

    printf("[DISK] Loading track %d...\n", track);
    printf("[DISK] DOS 3.3 physical order: ");
    for (int i = 0; i < 16; i++) {
        printf("%d%s", dos33_interleave[i], (i < 15) ? "," : "\n");
    }

    // Nibblize all 16 sectors using DOS 3.3 physical interleave
    for (int sector_index = 0; sector_index < 16; sector_index++) {
        int logical_sector = dos33_interleave[sector_index];

        // Calculate offset for this sector within the disk image (ProDOS block order)
        int offset = (track * 16 + logical_sector) * 256;

        if (offset + 256 > (int)disk_ctrl.disk_image->disk_size) {
            memset(disk_ctrl.sector_buffer, 0, 256);
        } else {
            // Copy raw sector data
            memcpy(disk_ctrl.sector_buffer,
                   disk_ctrl.disk_image->disk_data + offset,
                   256);
        }

        // Nibblize this sector into the track buffer
        MEM_WORD temp_nibbles[500];
        int nibble_count = nibblize_sector(
            disk_ctrl.sector_buffer,
            track,
            logical_sector,
            254,  // Volume number
            temp_nibbles
        );

        // Copy nibbles to track buffer
        if (pos + nibble_count <= (int)sizeof(disk_ctrl.track_nibbles)) {
            memcpy(&disk_ctrl.track_nibbles[pos], temp_nibbles, nibble_count);
            pos += nibble_count;
        } else {
            printf("[DISK] Warning: Track buffer full, truncating sector %d\n", logical_sector);
            break;
        }

        if (sector_index == 0) {
            printf("[DISK]   Physical %2d -> Logical %2d: offset=$%04X, %d nibbles, data: %02X %02X %02X %02X\n",
                   sector_index, logical_sector, offset, nibble_count,
                   disk_ctrl.sector_buffer[0], disk_ctrl.sector_buffer[1],
                   disk_ctrl.sector_buffer[2], disk_ctrl.sector_buffer[3]);
        }
    }

    disk_ctrl.track_size = pos;
    disk_ctrl.track_loaded = 1;
    disk_ctrl.byte_offset = 0;

    printf("[DISK] Track %d loaded: %d total nibbles\n", track, disk_ctrl.track_size);

    // Show first 500 nibbles for debugging
    int dump_len = disk_ctrl.track_size < 500 ? disk_ctrl.track_size : 500;
    printf("[DISK] First %d nibbles:\n", dump_len);
    for (int i = 0; i < dump_len; i++) {
        if (i % 32 == 0) {
            printf("  %04d: ", i);
        }
        printf("%02X ", disk_ctrl.track_nibbles[i]);
        if ((i % 32) == 31 || i == dump_len - 1) {
            printf("\n");
        }
    }

    // Look for prologues
    int addr_prologue_found = 0, data_prologue_found = 0;
    for (int i = 0; i < disk_ctrl.track_size - 2; i++) {
        if (disk_ctrl.track_nibbles[i] == 0xD5 &&
            disk_ctrl.track_nibbles[i+1] == 0xAA &&
            disk_ctrl.track_nibbles[i+2] == 0x96) {
            if (!addr_prologue_found) {
                printf("[DISK] Address prologue (D5 AA 96) found at offset %d\n", i);
                addr_prologue_found = 1;
            }
        }
        if (disk_ctrl.track_nibbles[i] == 0xD5 &&
            disk_ctrl.track_nibbles[i+1] == 0xAA &&
            disk_ctrl.track_nibbles[i+2] == 0xAD) {
            if (!data_prologue_found) {
                printf("[DISK] Data prologue (D5 AA AD) found at offset %d\n", i);
                data_prologue_found = 1;
                break;
            }
        }
    }
}

// Update track position based on phase magnet state
static void update_track_position(void) {
    // Simple stepper motor simulation
    // Each track uses 2 phases (quarter-track positioning)
    int active_phase = -1;

    for (int i = 0; i < 4; i++) {
        if (disk_ctrl.phases[i]) {
            active_phase = i;
            break;
        }
    }

    if (active_phase >= 0) {
        int new_track = active_phase * 2;  // Simplified: each phase = 2 tracks
        if (new_track != disk_ctrl.current_track && new_track < 35) {
            disk_ctrl.current_track = new_track;
            disk_ctrl.track_loaded = 0;  // Invalidate track on track change
            disk_ctrl.byte_offset = 0;

            #ifdef DEBUG_DISK
            printf("Seek to track %d (phase %d)\n", disk_ctrl.current_track, active_phase);
            #endif
        }
    }
}

MEM_WORD disk_controller_read(MEM_TWO_WORDS addr, void* ctx) {
    (void)ctx;

    int offset = addr & 0x0F;

    switch (offset) {
        case 0x0:  // Phase 0 off
        case 0x1:  // Phase 0 on
        case 0x2:  // Phase 1 off
        case 0x3:  // Phase 1 on
        case 0x4:  // Phase 2 off
        case 0x5:  // Phase 2 on
        case 0x6:  // Phase 3 off
        case 0x7: {// Phase 3 on
            int phase = offset / 2;
            disk_ctrl.phases[phase] = (offset & 1);
            update_track_position();
            return 0x00;
        }

        case 0x8:  // Motor off
            disk_ctrl.motor_on = 0;
            printf("[DISK] Motor OFF\n");
            return 0x00;

        case 0x9:  // Motor on
            disk_ctrl.motor_on = 1;
            printf("[DISK] Motor ON\n");
            return 0x00;

        case 0xA:  // Drive 1 select
            disk_ctrl.drive_selected = 0;
            return 0x00;

        case 0xB:  // Drive 2 select
            disk_ctrl.drive_selected = 1;
            return 0x00;

        case 0xC:  // Q6L - Read
            disk_ctrl.q6 = 0;
            if (!disk_ctrl.motor_on) {
                return 0x00;  // No data when drive is stopped
            }
            // When Q6L and Q7L, reading returns nibblized track data
            if (disk_ctrl.q7 == 0) {
                disk_ctrl.read_mode = 1;

                // Load track if needed
                if (!disk_ctrl.track_loaded) {
                    load_track(disk_ctrl.current_track);
                }

                // Return next nibble from track
                if (disk_ctrl.track_size > 0) {
                    MEM_WORD data = disk_ctrl.track_nibbles[disk_ctrl.byte_offset];

                    static int read_count = 0;
                    if (read_count < 200) {  // Increased to see full sector read
                        printf("[DISK_READ] PC=$%04X Track %d, offset %d/%d, data=$%02X\n",
                               REG.PC, disk_ctrl.current_track, disk_ctrl.byte_offset,
                               disk_ctrl.track_size, data);
                        read_count++;
                    }

                    // Advance and wrap around (simulate continuous disk rotation)
                    disk_ctrl.byte_offset = (disk_ctrl.byte_offset + 1) % disk_ctrl.track_size;

                    return data;
                }
            }
            return 0x00;

        case 0xD:  // Q6H - Sense write protect
            disk_ctrl.q6 = 1;
            return 0x00;  // Not write protected

        case 0xE:  // Q7L - Read mode
            disk_ctrl.q7 = 0;
            disk_ctrl.read_mode = 1;
            return 0x00;

        case 0xF:  // Q7H - Write mode
            disk_ctrl.q7 = 1;
            disk_ctrl.read_mode = 0;
            return 0x00;

        default:
            return 0x00;
    }
}

void disk_controller_write(MEM_TWO_WORDS addr, MEM_WORD data, void* ctx) {
    (void)data;

    // Most disk operations are done via reads; writes are for mode switching
    disk_controller_read(addr, ctx);
}
