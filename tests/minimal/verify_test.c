#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "loader.h"

int main() {
    printf("C99-6502 Verifier Host\n");

    // Initialize memory regions
    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);  // 32KB RAM
    mem_region_add_rom(0x8000, 0x8000);  // 32KB ROM

    // Load ROM and setup
    load_bin_region("tests/minimal/test.bin", 0x8000);
    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, 0x8000);
    mem_region_init();
    cpu_reset();

    // Running the test ROM
    cpu_run(1000);

    // Verify results
    int passed = 0;
    int failed = 0;

    printf("Test Results:\n\n");

    // Load and Store
    MEM_WORD val = bus_read(0x0200);
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

    printf("Total: %d passed, %d failed\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}