#ifndef DISK_IMAGE_H
#define DISK_IMAGE_H

#include "types.h"
#include <stdio.h>

typedef struct {
    char signature[4];
    MEM_TWO_WORDS creator;
    MEM_TWO_WORDS header_len;
    MEM_TWO_WORDS version;
    MEM_TWO_WORDS image_format;
    MEM_TWO_WORDS flags;
    MEM_TWO_WORDS blocks;
    MEM_TWO_WORDS data_offset;
    MEM_TWO_WORDS data_len;
    MEM_TWO_WORDS comment_offset;
    MEM_TWO_WORDS comment_len;
    MEM_TWO_WORDS creator_offset;
    MEM_TWO_WORDS creator_len;
} TWO_IMG_HEADER;

typedef struct {
    FILE* fp;
    TWO_IMG_HEADER header;
    MEM_WORD* disk_data;
    unsigned int disk_size;  // Changed from MEM_TWO_WORDS to support large disks
} DISK_IMAGE;

int disk_image_open(DISK_IMAGE* img, const char* filename);
void disk_image_close(DISK_IMAGE* img);
int disk_image_read_block(DISK_IMAGE* img, MEM_TWO_WORDS block, MEM_WORD* buffer);
void disk_image_info(DISK_IMAGE* img);

#endif
