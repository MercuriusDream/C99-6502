#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "loader.h"

static int passed = 0;
static int failed = 0;

void test_basic_6502() {
    printf("=== Basic 6502 Tests ===\n\n");

    // Initialize memory regions
    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);
    mem_region_add_rom(0x8000, 0x8000);

    load_bin_region("tests/minimal/test.bin", 0x8000);
    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, 0x8000);
    mem_region_init();
    cpu_reset();

    cpu_run(1000);

    MEM_WORD val;

    val = bus_read(0x0200);
    printf("Test 1 - LDA/STA: $0200 = $%02X (expected $42) ... %s\n",
           val, (val == 0x42) ? "PASS" : "FAIL");
    (val == 0x42) ? passed++ : failed++;

    val = bus_read(0x0201);
    printf("Test 2 - LDX/STX: $0201 = $%02X (expected $10) ... %s\n",
           val, (val == 0x10) ? "PASS" : "FAIL");
    (val == 0x10) ? passed++ : failed++;

    val = bus_read(0x0202);
    printf("Test 3 - LDY/STY: $0202 = $%02X (expected $20) ... %s\n",
           val, (val == 0x20) ? "PASS" : "FAIL");
    (val == 0x20) ? passed++ : failed++;

    // Arithmetic
    val = bus_read(0x0203);
    printf("Test 4 - ADC:     $0203 = $%02X (expected $15) ... %s\n",
           val, (val == 0x15) ? "PASS" : "FAIL");
    (val == 0x15) ? passed++ : failed++;

    val = bus_read(0x0204);
    printf("Test 5 - SBC:     $0204 = $%02X (expected $12) ... %s\n",
           val, (val == 0x12) ? "PASS" : "FAIL");
    (val == 0x12) ? passed++ : failed++;

    // Logical operations
    val = bus_read(0x0205);
    printf("Test 6 - AND:     $0205 = $%02X (expected $03) ... %s\n",
           val, (val == 0x03) ? "PASS" : "FAIL");
    (val == 0x03) ? passed++ : failed++;

    val = bus_read(0x0206);
    printf("Test 7 - ORA:     $0206 = $%02X (expected $07) ... %s\n",
           val, (val == 0x07) ? "PASS" : "FAIL");
    (val == 0x07) ? passed++ : failed++;

    val = bus_read(0x0207);
    printf("Test 8 - EOR:     $0207 = $%02X (expected $F0) ... %s\n",
           val, (val == 0xF0) ? "PASS" : "FAIL");
    (val == 0xF0) ? passed++ : failed++;

    // Increment/Decrement
    val = bus_read(0x0208);
    printf("Test 9 - INX:     $0208 = $%02X (expected $03) ... %s\n",
           val, (val == 0x03) ? "PASS" : "FAIL");
    (val == 0x03) ? passed++ : failed++;

    val = bus_read(0x0209);
    printf("Test 10 - DEY:    $0209 = $%02X (expected $03) ... %s\n",
           val, (val == 0x03) ? "PASS" : "FAIL");
    (val == 0x03) ? passed++ : failed++;

    // Branching
    val = bus_read(0x020A);
    printf("Test 11 - BEQ:    $020A = $%02X (expected $AA) ... %s\n",
           val, (val == 0xAA) ? "PASS" : "FAIL");
    (val == 0xAA) ? passed++ : failed++;

    // Subroutine
    val = bus_read(0x020B);
    printf("Test 12 - JSR/RTS: $020B = $%02X (expected $06) ... %s\n",
           val, (val == 0x06) ? "PASS" : "FAIL");
    (val == 0x06) ? passed++ : failed++;

    // Stack
    val = bus_read(0x020C);
    printf("Test 13 - PHA/PLA: $020C = $%02X (expected $55) ... %s\n",
           val, (val == 0x55) ? "PASS" : "FAIL");
    (val == 0x55) ? passed++ : failed++;

    // Indexed addressing
    val = bus_read(0x020D);
    printf("Test 14 - ABS,X:  $020D = $%02X (expected $44) ... %s\n",
           val, (val == 0x44) ? "PASS" : "FAIL");
    (val == 0x44) ? passed++ : failed++;

    // Zero page indexed
    val = bus_read(0x020E);
    printf("Test 15 - ZP,X:   $020E = $%02X (expected $99) ... %s\n",
           val, (val == 0x99) ? "PASS" : "FAIL");
    (val == 0x99) ? passed++ : failed++;
}

void test_65c02_fixes() {
    printf("\n=== 65C02 Fix Tests ===\n\n");

    mem_region_clear();
    mem_region_add_ram(0x0000, 0xFFFF);
    mem_region_init();
    cpu_set_variant(CPU_VARIANT_CMOS_65C02);
    cpu_reset();

    MEM_WORD val;

    // Test BRA (unconditional branch)
    REG.PC = 0x1000;
    bus_write(0x1000, 0x80);
    bus_write(0x1001, 0x02);
    cpu_step();
    printf("Test 16 - BRA:    PC = $%04X (expected $1004) ... %s\n",
           REG.PC, (REG.PC == 0x1004) ? "PASS" : "FAIL");
    (REG.PC == 0x1004) ? passed++ : failed++;

    // Test BIT #imm (should not affect N/V flags)
    REG.PC = 0x2000;
    REG.A = 0x00;
    REG.P = 0;
    bus_write(0x2000, 0x89);
    bus_write(0x2001, 0x80);
    cpu_step();
    val = GET_FLAG(FLAG_Z) && !GET_FLAG(FLAG_N) && !GET_FLAG(FLAG_V);
    printf("Test 17 - BIT #:  Z=%d N=%d V=%d (expected Z=1 N=0 V=0) ... %s\n",
           GET_FLAG(FLAG_Z), GET_FLAG(FLAG_N), GET_FLAG(FLAG_V),
           val ? "PASS" : "FAIL");
    val ? passed++ : failed++;

    // Test JMP (abs,x)
    REG.PC = 0x3000;
    REG.X = 0x04;
    bus_write(0x3000, 0x7C);
    bus_write(0x3001, 0x00);
    bus_write(0x3002, 0x40);
    bus_write(0x4004, 0x00);
    bus_write(0x4005, 0x50);
    cpu_step();
    printf("Test 18 - JMP(A,X): PC = $%04X (expected $5000) ... %s\n",
           REG.PC, (REG.PC == 0x5000) ? "PASS" : "FAIL");
    (REG.PC == 0x5000) ? passed++ : failed++;

    // Test ORA (zp)
    REG.PC = 0x6000;
    REG.A = 0x55;
    bus_write(0x6000, 0x12);
    bus_write(0x6001, 0x10);
    bus_write(0x0010, 0x00);
    bus_write(0x0011, 0x70);
    bus_write(0x7000, 0xAA);
    cpu_step();
    printf("Test 19 - ORA(ZP): A = $%02X (expected $FF) ... %s\n",
           REG.A, (REG.A == 0xFF) ? "PASS" : "FAIL");
    (REG.A == 0xFF) ? passed++ : failed++;
}

int main() {
    printf("C99-6502 Verifier Host\n\n");

    test_basic_6502();
    test_65c02_fixes();

    printf("\n=== Total: %d passed, %d failed ===\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}