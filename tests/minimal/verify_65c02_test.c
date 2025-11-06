#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "loader.h"

int main() {
    printf("C99-6502 65C02 Verifier\n");

    cpu_set_variant(CPU_VARIANT_CMOS_65C02);

    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);
    mem_region_add_rom(0x8000, 0x8000);

    load_bin_region("tests/minimal/65c02_test.bin", 0x8000);
    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, 0x8000);
    mem_region_init();
    cpu_reset();
    cpu_run(100000);

    printf("\n65C02 Test Results:\n\n");

    int passed = 0;
    int failed = 0;

    MEM_WORD val;

    val = bus_read(0x0200);
    printf("Test 1 - BRA:     $0200 = $%02X (expected $01) ... %s\n",
           val, (val == 0x01) ? "PASS" : "FAIL");
    (val == 0x01) ? passed++ : failed++;

    val = bus_read(0x0201);
    printf("Test 2 - PHX/PLX: $0201 = $%02X (expected $42) ... %s\n",
           val, (val == 0x42) ? "PASS" : "FAIL");
    (val == 0x42) ? passed++ : failed++;

    val = bus_read(0x0202);
    printf("Test 3 - PHY/PLY: $0202 = $%02X (expected $43) ... %s\n",
           val, (val == 0x43) ? "PASS" : "FAIL");
    (val == 0x43) ? passed++ : failed++;

    val = bus_read(0x0203);
    printf("Test 4 - STZ ZP:  $0203 = $%02X (expected $00) ... %s\n",
           val, (val == 0x00) ? "PASS" : "FAIL");
    (val == 0x00) ? passed++ : failed++;

    val = bus_read(0x0204);
    printf("Test 5 - STZ ABS: $0204 = $%02X (expected $00) ... %s\n",
           val, (val == 0x00) ? "PASS" : "FAIL");
    (val == 0x00) ? passed++ : failed++;

    val = bus_read(0x0205);
    printf("Test 6 - TSB:     $0205 = $%02X (expected $FF) ... %s\n",
           val, (val == 0xFF) ? "PASS" : "FAIL");
    (val == 0xFF) ? passed++ : failed++;

    val = bus_read(0x0206);
    printf("Test 7 - TRB:     $0206 = $%02X (expected $0F) ... %s\n",
           val, (val == 0x0F) ? "PASS" : "FAIL");
    (val == 0x0F) ? passed++ : failed++;

    val = bus_read(0x0207);
    printf("Test 8 - STZ ZPX: $0207 = $%02X (expected $00) ... %s\n",
           val, (val == 0x00) ? "PASS" : "FAIL");
    (val == 0x00) ? passed++ : failed++;

    val = bus_read(0x020B);
    printf("Test 9 - STZ ABX: $020B = $%02X (expected $00) ... %s\n",
           val, (val == 0x00) ? "PASS" : "FAIL");
    (val == 0x00) ? passed++ : failed++;

    printf("\nTotal: %d passed, %d failed\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}
