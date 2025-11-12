#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "assembler.h"

static void print_usage(const char* prog_name) {
    printf("Usage: %s [options] <input.asm>\n", prog_name);
    printf("Options:\n");
    printf("  -o, --output <file>    Output binary file (default: out.bin)\n");
    printf("  -c, --cpu <variant>    CPU variant: 6502, 65c02 (default: 6502)\n");
    printf("  -l, --listing          Print assembly listing\n");
    printf("  -h, --help             Show this help\n");
}

int main(int argc, char** argv) {
    char* input_file = NULL;
    char* output_file = "out.bin";
    CPU_VARIANT variant = CPU_VARIANT_NMOS_6502;
    int show_listing = 0;

    static struct option long_options[] = {
        {"output",  required_argument, 0, 'o'},
        {"cpu",     required_argument, 0, 'c'},
        {"listing", no_argument,       0, 'l'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:c:lh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'o':
                output_file = optarg;
                break;
            case 'c':
                if (strcmp(optarg, "65c02") == 0 || strcmp(optarg, "cmos") == 0) {
                    variant = CPU_VARIANT_CMOS_65C02;
                } else {
                    variant = CPU_VARIANT_NMOS_6502;
                }
                break;
            case 'l':
                show_listing = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    input_file = argv[optind];

    printf("=== MOS 6502 Assembler ===\n");
    printf("Input:  %s\n", input_file);
    printf("Output: %s\n", output_file);
    printf("CPU:    %s\n\n", variant == CPU_VARIANT_CMOS_65C02 ? "65C02" : "6502");

    ASSEMBLER assembler;
    asm_init(&assembler, variant);

    printf("Pass 1: Collecting labels...\n");
    printf("Pass 2: Generating code...\n");

    if (asm_assemble_file(&assembler, input_file) < 0) {
        fprintf(stderr, "Assembly failed\n");
        return 1;
    }

    if (show_listing) {
        asm_print_listing(&assembler);
    }

    if (asm_write_binary(&assembler, output_file) < 0) {
        return 1;
    }

    printf("\nAssembly successful!\n");
    return 0;
}
