#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "types.h"

#define MAX_LABEL_LEN 64
#define MAX_LINE_LEN 256
#define MAX_LABELS 1024
#define MAX_CODE_SIZE 65536

typedef struct {
    char name[MAX_LABEL_LEN];
    MEM_TWO_WORDS address;
} LABEL;

typedef struct {
    LABEL labels[MAX_LABELS];
    int label_count;
    MEM_WORD code[MAX_CODE_SIZE];
    MEM_TWO_WORDS code_size;
    MEM_TWO_WORDS origin;
    CPU_VARIANT cpu_variant;
} ASSEMBLER;

void asm_init(ASSEMBLER* assembler, CPU_VARIANT variant);
int asm_assemble_file(ASSEMBLER* assembler, const char* filename);
int asm_assemble_string(ASSEMBLER* assembler, const char* source);
int asm_write_binary(ASSEMBLER* assembler, const char* filename);
void asm_print_listing(ASSEMBLER* assembler);

#endif
