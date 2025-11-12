#define _POSIX_C_SOURCE 200809L
#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char* mnemonic;
    MEM_WORD opcode_imp;
    MEM_WORD opcode_acc;
    MEM_WORD opcode_imm;
    MEM_WORD opcode_zp;
    MEM_WORD opcode_zpx;
    MEM_WORD opcode_zpy;
    MEM_WORD opcode_abs;
    MEM_WORD opcode_absx;
    MEM_WORD opcode_absy;
    MEM_WORD opcode_ind;
    MEM_WORD opcode_indx;
    MEM_WORD opcode_indy;
    MEM_WORD opcode_rel;
} OPCODE_TABLE_ENTRY;

static const OPCODE_TABLE_ENTRY OPCODE_TABLE[] = {
    {"ADC", 0xFF, 0xFF, 0x69, 0x65, 0x75, 0xFF, 0x6D, 0x7D, 0x79, 0xFF, 0x61, 0x71, 0xFF},
    {"AND", 0xFF, 0xFF, 0x29, 0x25, 0x35, 0xFF, 0x2D, 0x3D, 0x39, 0xFF, 0x21, 0x31, 0xFF},
    {"ASL", 0xFF, 0x0A, 0xFF, 0x06, 0x16, 0xFF, 0x0E, 0x1E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"BCC", 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x90},
    {"BCS", 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB0},
    {"BEQ", 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0},
    {"BIT", 0xFF, 0xFF, 0xFF, 0x24, 0xFF, 0xFF, 0x2C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"BMI", 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x30},
    {"BNE", 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xD0},
    {"BPL", 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x10},
    {"BRA", 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80},
    {"BRK", 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"BVC", 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x50},
    {"BVS", 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x70},
    {"CLC", 0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"CLD", 0xD8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"CLI", 0x58, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"CLV", 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"CMP", 0xFF, 0xFF, 0xC9, 0xC5, 0xD5, 0xFF, 0xCD, 0xDD, 0xD9, 0xFF, 0xC1, 0xD1, 0xFF},
    {"CPX", 0xFF, 0xFF, 0xE0, 0xE4, 0xFF, 0xFF, 0xEC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"CPY", 0xFF, 0xFF, 0xC0, 0xC4, 0xFF, 0xFF, 0xCC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"DEC", 0xFF, 0xFF, 0xFF, 0xC6, 0xD6, 0xFF, 0xCE, 0xDE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"DEX", 0xCA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"DEY", 0x88, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"EOR", 0xFF, 0xFF, 0x49, 0x45, 0x55, 0xFF, 0x4D, 0x5D, 0x59, 0xFF, 0x41, 0x51, 0xFF},
    {"INC", 0xFF, 0xFF, 0xFF, 0xE6, 0xF6, 0xFF, 0xEE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"INX", 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"INY", 0xC8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"JMP", 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4C, 0xFF, 0xFF, 0x6C, 0xFF, 0xFF, 0xFF},
    {"JSR", 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"LDA", 0xFF, 0xFF, 0xA9, 0xA5, 0xB5, 0xFF, 0xAD, 0xBD, 0xB9, 0xFF, 0xA1, 0xB1, 0xFF},
    {"LDX", 0xFF, 0xFF, 0xA2, 0xA6, 0xFF, 0xB6, 0xAE, 0xFF, 0xBE, 0xFF, 0xFF, 0xFF, 0xFF},
    {"LDY", 0xFF, 0xFF, 0xA0, 0xA4, 0xB4, 0xFF, 0xAC, 0xBC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"LSR", 0xFF, 0x4A, 0xFF, 0x46, 0x56, 0xFF, 0x4E, 0x5E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"NOP", 0xEA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"ORA", 0xFF, 0xFF, 0x09, 0x05, 0x15, 0xFF, 0x0D, 0x1D, 0x19, 0xFF, 0x01, 0x11, 0xFF},
    {"PHA", 0x48, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"PHP", 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"PHX", 0xDA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"PHY", 0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"PLA", 0x68, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"PLP", 0x28, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"PLX", 0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"PLY", 0x7A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"ROL", 0xFF, 0x2A, 0xFF, 0x26, 0x36, 0xFF, 0x2E, 0x3E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"ROR", 0xFF, 0x6A, 0xFF, 0x66, 0x76, 0xFF, 0x6E, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"RTI", 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"RTS", 0x60, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"SBC", 0xFF, 0xFF, 0xE9, 0xE5, 0xF5, 0xFF, 0xED, 0xFD, 0xF9, 0xFF, 0xE1, 0xF1, 0xFF},
    {"SEC", 0x38, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"SED", 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"SEI", 0x78, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"STA", 0xFF, 0xFF, 0xFF, 0x85, 0x95, 0xFF, 0x8D, 0x9D, 0x99, 0xFF, 0x81, 0x91, 0xFF},
    {"STP", 0xDB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"STX", 0xFF, 0xFF, 0xFF, 0x86, 0xFF, 0x96, 0x8E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"STY", 0xFF, 0xFF, 0xFF, 0x84, 0x94, 0xFF, 0x8C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"STZ", 0xFF, 0xFF, 0xFF, 0x64, 0x74, 0xFF, 0x9C, 0x9E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"TAX", 0xAA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"TAY", 0xA8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"TRB", 0xFF, 0xFF, 0xFF, 0x14, 0xFF, 0xFF, 0x1C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"TSB", 0xFF, 0xFF, 0xFF, 0x04, 0xFF, 0xFF, 0x0C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"TSX", 0xBA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"TXA", 0x8A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"TXS", 0x9A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"TYA", 0x98, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {"WAI", 0xCB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};


// Helpers


static void trim(char* str) {
    char* start = str;
    char* end;

    while (isspace((unsigned char)*start)) start++;
    if (*start == 0) {
        *str = 0;
        return;
    }

    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;

    memmove(str, start, end - start + 1);
    str[end - start + 1] = '\0';
}

static void remove_comment(char* line) {
    char* comment = strchr(line, ';');
    if (comment) *comment = '\0';
}

static void to_upper(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

static const OPCODE_TABLE_ENTRY* find_opcode(const char* mnemonic) {
    for (int i = 0; OPCODE_TABLE[i].mnemonic != NULL; i++) {
        if (strcmp(OPCODE_TABLE[i].mnemonic, mnemonic) == 0) {
            return &OPCODE_TABLE[i];
        }
    }
    return NULL;
}

static int is_label_line(const char* line) {
    int len = strlen(line);
    return len > 0 && line[len - 1] == ':';
}

static int find_label(ASSEMBLER* asm_ctx, const char* name, MEM_TWO_WORDS* addr) {
    for (int i = 0; i < asm_ctx->label_count; i++) {
        if (strcmp(asm_ctx->labels[i].name, name) == 0) {
            *addr = asm_ctx->labels[i].address;
            return 1;
        }
    }
    return 0;
}

static void add_label(ASSEMBLER* asm_ctx, const char* name, MEM_TWO_WORDS address) {
    if (asm_ctx->label_count >= MAX_LABELS) {
        fprintf(stderr, "Error: Too many labels\n");
        return;
    }
    strncpy(asm_ctx->labels[asm_ctx->label_count].name, name, MAX_LABEL_LEN - 1);
    asm_ctx->labels[asm_ctx->label_count].address = address;
    asm_ctx->label_count++;
}

static void emit_byte(ASSEMBLER* asm_ctx, MEM_WORD byte) {
    asm_ctx->code[asm_ctx->code_size++] = byte;
}

static void emit_word(ASSEMBLER* asm_ctx, MEM_TWO_WORDS word) {
    emit_byte(asm_ctx, word & 0xFF);
    emit_byte(asm_ctx, (word >> 8) & 0xFF);
}

static int parse_number(const char* str, MEM_TWO_WORDS* value) {
    if (str[0] == '$') {
        *value = (MEM_TWO_WORDS)strtol(str + 1, NULL, 16);
        return 1;
    } else if (str[0] >= '0' && str[0] <= '9') {
        *value = (MEM_TWO_WORDS)strtol(str, NULL, 10);
        return 1;
    }
    return 0;
}


// Assembly


static int assemble_line(ASSEMBLER* asm_ctx, char* line, int pass) {
    char mnemonic[MAX_LABEL_LEN];
    char operand[MAX_LABEL_LEN];

    remove_comment(line);
    trim(line);

    if (strlen(line) == 0) return 0;

    if (strncmp(line, ".ORG", 4) == 0 || strncmp(line, ".org", 4) == 0) {
        MEM_TWO_WORDS addr;
        sscanf(line + 4, "%s", operand);
        if (parse_number(operand, &addr)) {
            asm_ctx->origin = addr;
            if (pass == 1) asm_ctx->code_size = 0;
        }
        return 0;
    }

    if (is_label_line(line)) {
        char label[MAX_LABEL_LEN];
        strncpy(label, line, strlen(line) - 1);
        label[strlen(line) - 1] = '\0';
        to_upper(label);
        if (pass == 1) {
            add_label(asm_ctx, label, asm_ctx->origin + asm_ctx->code_size);
        }
        return 0;
    }

    if (sscanf(line, "%s %[^\n]", mnemonic, operand) < 1) {
        return 0;
    }

    to_upper(mnemonic);

    const OPCODE_TABLE_ENTRY* op = find_opcode(mnemonic);
    if (!op) {
        fprintf(stderr, "Error: Unknown mnemonic '%s'\n", mnemonic);
        return -1;
    }

    if (sscanf(line, "%s %[^\n]", mnemonic, operand) == 1) {
        if (op->opcode_imp != 0xFF) {
            if (pass == 2) emit_byte(asm_ctx, op->opcode_imp);
            else asm_ctx->code_size += 1;
            return 0;
        } else if (op->opcode_acc != 0xFF) {
            if (pass == 2) emit_byte(asm_ctx, op->opcode_acc);
            else asm_ctx->code_size += 1;
            return 0;
        }
    }

    trim(operand);

    if (operand[0] == '#') {
        MEM_TWO_WORDS value;
        if (parse_number(operand + 1, &value)) {
            if (op->opcode_imm != 0xFF) {
                if (pass == 2) {
                    emit_byte(asm_ctx, op->opcode_imm);
                    emit_byte(asm_ctx, value & 0xFF);
                } else {
                    asm_ctx->code_size += 2;
                }
                return 0;
            }
        }
    } else if (operand[0] == '(' && strchr(operand, ',')) {
        char addr_str[MAX_LABEL_LEN];
        sscanf(operand, "(%[^,],X)", addr_str);
        MEM_TWO_WORDS addr;
        if (parse_number(addr_str, &addr) || find_label(asm_ctx, addr_str, &addr)) {
            if (op->opcode_indx != 0xFF) {
                if (pass == 2) {
                    emit_byte(asm_ctx, op->opcode_indx);
                    emit_byte(asm_ctx, addr & 0xFF);
                } else {
                    asm_ctx->code_size += 2;
                }
                return 0;
            }
        }
    } else if (operand[0] == '(' && operand[strlen(operand)-1] == ')') {
        char addr_str[MAX_LABEL_LEN];
        sscanf(operand, "(%[^)])", addr_str);
        MEM_TWO_WORDS addr;
        if (parse_number(addr_str, &addr) || find_label(asm_ctx, addr_str, &addr)) {
            if (strchr(operand, ',')) {
                if (op->opcode_indy != 0xFF) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, op->opcode_indy);
                        emit_byte(asm_ctx, addr & 0xFF);
                    } else {
                        asm_ctx->code_size += 2;
                    }
                    return 0;
                }
            } else {
                if (op->opcode_ind != 0xFF) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, op->opcode_ind);
                        emit_word(asm_ctx, addr);
                    } else {
                        asm_ctx->code_size += 3;
                    }
                    return 0;
                }
            }
        }
    } else if (strchr(operand, ',')) {
        char addr_str[MAX_LABEL_LEN];
        char reg;
        sscanf(operand, "%[^,],%c", addr_str, &reg);
        trim(addr_str);
        reg = toupper(reg);

        MEM_TWO_WORDS addr;
        if (parse_number(addr_str, &addr) || find_label(asm_ctx, addr_str, &addr)) {
            if (reg == 'X') {
                if (addr < 256 && op->opcode_zpx != 0xFF) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, op->opcode_zpx);
                        emit_byte(asm_ctx, addr & 0xFF);
                    } else {
                        asm_ctx->code_size += 2;
                    }
                } else if (op->opcode_absx != 0xFF) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, op->opcode_absx);
                        emit_word(asm_ctx, addr);
                    } else {
                        asm_ctx->code_size += 3;
                    }
                }
                return 0;
            } else if (reg == 'Y') {
                if (addr < 256 && op->opcode_zpy != 0xFF) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, op->opcode_zpy);
                        emit_byte(asm_ctx, addr & 0xFF);
                    } else {
                        asm_ctx->code_size += 2;
                    }
                } else if (op->opcode_absy != 0xFF) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, op->opcode_absy);
                        emit_word(asm_ctx, addr);
                    } else {
                        asm_ctx->code_size += 3;
                    }
                }
                return 0;
            }
        }
    } else {
        MEM_TWO_WORDS addr;
        int found = parse_number(operand, &addr) || find_label(asm_ctx, operand, &addr);

        if (found) {
            if (op->opcode_rel != 0xFF) {
                SIGNED_MEM_WORD offset = (SIGNED_MEM_WORD)(addr - (asm_ctx->origin + asm_ctx->code_size + 2));
                if (pass == 2) {
                    emit_byte(asm_ctx, op->opcode_rel);
                    emit_byte(asm_ctx, (MEM_WORD)offset);
                } else {
                    asm_ctx->code_size += 2;
                }
                return 0;
            }

            if (addr < 256 && op->opcode_zp != 0xFF) {
                if (pass == 2) {
                    emit_byte(asm_ctx, op->opcode_zp);
                    emit_byte(asm_ctx, addr & 0xFF);
                } else {
                    asm_ctx->code_size += 2;
                }
                return 0;
            }

            if (op->opcode_abs != 0xFF) {
                if (pass == 2) {
                    emit_byte(asm_ctx, op->opcode_abs);
                    emit_word(asm_ctx, addr);
                } else {
                    asm_ctx->code_size += 3;
                }
                return 0;
            }
        }
    }

    fprintf(stderr, "Error: Invalid addressing mode for '%s %s'\n", mnemonic, operand);
    return -1;
}


// Public Interface


void asm_init(ASSEMBLER* assembler, CPU_VARIANT variant) {
    memset(assembler, 0, sizeof(ASSEMBLER));
    assembler->origin = 0x8000;
    assembler->cpu_variant = variant;
}

int asm_assemble_string(ASSEMBLER* asm_ctx, const char* source) {
    char* source_copy = strdup(source);
    char* line = strtok(source_copy, "\n");

    asm_ctx->code_size = 0;

    while (line != NULL) {
        if (assemble_line(asm_ctx, line, 1) < 0) {
            free(source_copy);
            return -1;
        }
        line = strtok(NULL, "\n");
    }

    free(source_copy);
    source_copy = strdup(source);
    line = strtok(source_copy, "\n");
    asm_ctx->code_size = 0;

    while (line != NULL) {
        if (assemble_line(asm_ctx, line, 2) < 0) {
            free(source_copy);
            return -1;
        }
        line = strtok(NULL, "\n");
    }

    free(source_copy);
    return 0;
}

int asm_assemble_file(ASSEMBLER* asm_ctx, const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }

    char line[MAX_LINE_LEN];
    asm_ctx->code_size = 0;

    while (fgets(line, MAX_LINE_LEN, fp)) {
        if (assemble_line(asm_ctx, line, 1) < 0) {
            fclose(fp);
            return -1;
        }
    }

    fseek(fp, 0, SEEK_SET);
    asm_ctx->code_size = 0;

    while (fgets(line, MAX_LINE_LEN, fp)) {
        if (assemble_line(asm_ctx, line, 2) < 0) {
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}

int asm_write_binary(ASSEMBLER* asm_ctx, const char* filename) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create file '%s'\n", filename);
        return -1;
    }

    fwrite(asm_ctx->code, 1, asm_ctx->code_size, fp);
    fclose(fp);

    printf("Assembled %d bytes to '%s'\n", asm_ctx->code_size, filename);
    return 0;
}

void asm_print_listing(ASSEMBLER* asm_ctx) {
    printf("\nLabels:\n");
    for (int i = 0; i < asm_ctx->label_count; i++) {
        printf("  %s = $%04X\n", asm_ctx->labels[i].name, asm_ctx->labels[i].address);
    }

    printf("\nCode (%d bytes at $%04X):\n", asm_ctx->code_size, asm_ctx->origin);
    for (MEM_TWO_WORDS i = 0; i < asm_ctx->code_size; i += 16) {
        printf("  %04X: ", asm_ctx->origin + i);
        for (int j = 0; j < 16 && i + j < asm_ctx->code_size; j++) {
            printf("%02X ", asm_ctx->code[i + j]);
        }
        printf("\n");
    }
}
