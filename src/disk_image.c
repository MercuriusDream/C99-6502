#define _POSIX_C_SOURCE 200809L
#include "disk_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MEM_TWO_WORDS read_word(FILE* fp) {
    MEM_WORD low = fgetc(fp);
    MEM_WORD high = fgetc(fp);
    return (MEM_TWO_WORDS)((high << 8) | low);
}

static unsigned int read_dword(FILE* fp) {
    unsigned int b0 = fgetc(fp);
    unsigned int b1 = fgetc(fp);
    unsigned int b2 = fgetc(fp);
    unsigned int b3 = fgetc(fp);
    return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}

int disk_image_open(DISK_IMAGE* img, const char* filename) {
    img->fp = fopen(filename, "rb");
    if (!img->fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }

    fseek(img->fp, 0, SEEK_END);
    long file_size = ftell(img->fp);
    fseek(img->fp, 0, SEEK_SET);

    printf("[DISK_IMAGE] File size: %ld bytes\n", file_size);

    fread(img->header.signature, 1, 4, img->fp);

    if (memcmp(img->header.signature, "2IMG", 4) == 0) {
        fseek(img->fp, 0, SEEK_SET);
    } else {
        printf("Raw disk image detected (ProDOS .po or DOS .do)\n");
        memset(&img->header, 0, sizeof(img->header));
        img->header.image_format = 1;
        img->disk_size = file_size;
        printf("[DISK_IMAGE] Setting disk_size to: %u\n", img->disk_size);
        img->disk_data = (MEM_WORD*)malloc(img->disk_size);

        if (!img->disk_data) {
            fprintf(stderr, "Error: Cannot allocate disk buffer\n");
            fclose(img->fp);
            return -1;
        }

        fseek(img->fp, 0, SEEK_SET);
        size_t bytes_read = fread(img->disk_data, 1, img->disk_size, img->fp);

        if (bytes_read != img->disk_size) {
            fprintf(stderr, "Warning: Read %zu bytes, expected %u\n", bytes_read, img->disk_size);
        }

        printf("Loaded raw disk image:\n");
        printf("  Size: %u bytes (%u KB)\n", img->disk_size, img->disk_size / 1024);
        printf("  Blocks: %u\n", img->disk_size / 512);

        return 0;
    }

    img->header.creator = read_word(img->fp);
    img->header.header_len = read_word(img->fp);
    img->header.version = read_word(img->fp);
    img->header.image_format = read_word(img->fp);
    img->header.flags = read_word(img->fp);
    img->header.blocks = read_word(img->fp);
    img->header.data_offset = read_word(img->fp);
    img->header.data_len = read_word(img->fp);
    img->header.comment_offset = read_word(img->fp);
    img->header.comment_len = read_word(img->fp);
    img->header.creator_offset = read_word(img->fp);
    img->header.creator_len = read_word(img->fp);

    fseek(img->fp, 8, SEEK_SET);
    unsigned int blocks = read_dword(img->fp);
    fseek(img->fp, 12, SEEK_SET);
    unsigned int data_offset = read_dword(img->fp);
    unsigned int data_len = read_dword(img->fp);

    img->disk_size = (data_len > 0) ? data_len : (blocks * 512);
    img->disk_data = (MEM_WORD*)malloc(img->disk_size);

    if (!img->disk_data) {
        fprintf(stderr, "Error: Cannot allocate disk buffer\n");
        fclose(img->fp);
        return -1;
    }

    fseek(img->fp, data_offset, SEEK_SET);
    size_t bytes_read = fread(img->disk_data, 1, img->disk_size, img->fp);

    if (bytes_read != img->disk_size) {
        fprintf(stderr, "Warning: Read %zu bytes, expected %u\n", bytes_read, img->disk_size);
    }

    printf("Loaded 2IMG disk image:\n");
    printf("  Format: %u\n", img->header.image_format);
    printf("  Blocks: %u\n", blocks);
    printf("  Size: %u bytes\n", img->disk_size);

    return 0;
}

void disk_image_close(DISK_IMAGE* img) {
    if (img->fp) {
        fclose(img->fp);
        img->fp = NULL;
    }
    if (img->disk_data) {
        free(img->disk_data);
        img->disk_data = NULL;
    }
}

int disk_image_read_block(DISK_IMAGE* img, MEM_TWO_WORDS block, MEM_WORD* buffer) {
    if (block * 512 >= img->disk_size) {
        return -1;
    }
    memcpy(buffer, img->disk_data + (block * 512), 512);
    return 0;
}

void disk_image_info(DISK_IMAGE* img) {
    printf("\n=== Disk Image Info ===\n");
    printf("Format: ");
    switch (img->header.image_format) {
        case 0: printf("DOS 3.3\n"); break;
        case 1: printf("ProDOS\n"); break;
        case 2: printf("NIB\n"); break;
        default: printf("Unknown (%u)\n", img->header.image_format);
    }
    printf("Size: %u bytes (%u KB)\n", img->disk_size, img->disk_size / 1024);
    printf("Locked: %s\n", (img->header.flags & 0x80000000) ? "Yes" : "No");

    printf("\nFirst 256 bytes:\n");
    for (int i = 0; i < 256 && i < (int)img->disk_size; i += 16) {
        printf("  %04X: ", i);
        for (int j = 0; j < 16 && i + j < (int)img->disk_size; j++) {
            printf("%02X ", img->disk_data[i + j]);
        }
        printf("\n");
    }
}
