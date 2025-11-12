#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk_image.h"

static void print_usage(const char* prog) {
    printf("Usage: %s <disk.2mg> [options]\n", prog);
    printf("Options:\n");
    printf("  -i            Show disk info\n");
    printf("  -b <block>    Dump specific block\n");
    printf("  -e <addr>     Extract to raw binary at address\n");
    printf("  -s <size>     Size to extract (with -e)\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    DISK_IMAGE img;
    if (disk_image_open(&img, argv[1]) != 0) {
        return 1;
    }

    int show_info = 1;
    int dump_block = -1;
    int extract_addr = -1;
    int extract_size = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            show_info = 1;
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            dump_block = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            extract_addr = (int)strtol(argv[++i], NULL, 16);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            extract_size = atoi(argv[++i]);
        }
    }

    if (show_info) {
        disk_image_info(&img);
    }

    if (dump_block >= 0) {
        MEM_WORD buffer[512];
        if (disk_image_read_block(&img, dump_block, buffer) == 0) {
            printf("\nBlock %d:\n", dump_block);
            for (int i = 0; i < 512; i += 16) {
                printf("  %04X: ", i);
                for (int j = 0; j < 16; j++) {
                    printf("%02X ", buffer[i + j]);
                }
                printf(" | ");
                for (int j = 0; j < 16; j++) {
                    MEM_WORD c = buffer[i + j];
                    printf("%c", (c >= 32 && c < 127) ? c : '.');
                }
                printf("\n");
            }
        }
    }

    if (extract_addr >= 0 && extract_size > 0) {
        if (extract_addr < (int)img.disk_size) {
            char outfile[256];
            snprintf(outfile, sizeof(outfile), "extracted_%04X.bin", extract_addr);
            FILE* out = fopen(outfile, "wb");
            if (out) {
                int actual_size = extract_size;
                if (extract_addr + extract_size > (int)img.disk_size) {
                    actual_size = img.disk_size - extract_addr;
                }
                fwrite(img.disk_data + extract_addr, 1, actual_size, out);
                fclose(out);
                printf("\nExtracted %d bytes from offset $%04X to %s\n",
                       actual_size, extract_addr, outfile);
            }
        }
    }

    disk_image_close(&img);
    return 0;
}
