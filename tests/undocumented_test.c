#include <stdio.h>
#include <string.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"

// Test helper macros
#define ASSERT(condition, message) do { \
    if (!(condition)) { \
        printf("FAIL: %s\n", message); \
        return 0; \
    } \
} while(0)

#define TEST(name) int test_##name(void)

// Test helpers
void setup_test(void) {
    cpu_init();
    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);
    mem_region_add_rom(0x8000, 0x8000);
    mem_region_init();
}

void write_byte(MEM_TWO_WORDS addr, MEM_WORD value) {
    bus_write(addr, value);
}

MEM_WORD read_byte(MEM_TWO_WORDS addr) {
    return bus_read(addr);
}


// Test 1: ANC - AND with Carry (0x0B, 0x2B)
// Operation: A = A AND immediate; C = N (bit 7)

TEST(ANC_sets_carry_from_bit7) {
    setup_test();

    // Set up: A = 0xFF, ANC #$80 -> A = 0x80, C = 1 (bit 7 set)
    REG.A = 0xFF;
    REG.P = 0x00;

    write_byte(0x0400, 0x0B);  // ANC immediate
    write_byte(0x0401, 0x80);  // Operand

    REG.PC = 0x0400;
    cpu_step();

    ASSERT(REG.A == 0x80, "ANC: A should be 0x80");
    ASSERT(GET_FLAG(FLAG_C), "ANC: Carry should be set when bit 7 is 1");
    ASSERT(GET_FLAG(FLAG_N), "ANC: Negative flag should be set");
    ASSERT(!GET_FLAG(FLAG_Z), "ANC: Zero flag should be clear");

    printf("PASS: ANC sets carry from bit 7 (high)\n");
    return 1;
}

TEST(ANC_clears_carry_when_bit7_clear) {
    setup_test();

    // Set up: A = 0xFF, ANC #$7F -> A = 0x7F, C = 0 (bit 7 clear)
    REG.A = 0xFF;
    REG.P = 0xFF;  // All flags set

    write_byte(0x0400, 0x2B);  // ANC immediate (alternate opcode)
    write_byte(0x0401, 0x7F);  // Operand

    REG.PC = 0x0400;
    cpu_step();

    ASSERT(REG.A == 0x7F, "ANC: A should be 0x7F");
    ASSERT(!GET_FLAG(FLAG_C), "ANC: Carry should be clear when bit 7 is 0");
    ASSERT(!GET_FLAG(FLAG_N), "ANC: Negative flag should be clear");
    ASSERT(!GET_FLAG(FLAG_Z), "ANC: Zero flag should be clear");

    printf("PASS: ANC clears carry when bit 7 is clear\n");
    return 1;
}


// Test 2: ALR - AND then LSR (0x4B)
// Operation: A = (A AND immediate) >> 1

TEST(ALR_and_then_shift_right) {
    setup_test();

    // Set up: A = 0xFF, ALR #$FE -> A = (0xFF & 0xFE) >> 1 = 0x7F, C = 0
    REG.A = 0xFF;
    REG.P = 0x00;

    write_byte(0x0400, 0x4B);  // ALR immediate
    write_byte(0x0401, 0xFE);  // Operand

    REG.PC = 0x0400;
    cpu_step();

    ASSERT(REG.A == 0x7F, "ALR: A should be 0x7F");
    ASSERT(!GET_FLAG(FLAG_C), "ALR: Carry should be clear (bit 0 was 0)");
    ASSERT(!GET_FLAG(FLAG_N), "ALR: Negative flag should be clear");
    ASSERT(!GET_FLAG(FLAG_Z), "ALR: Zero flag should be clear");

    printf("PASS: ALR performs AND then LSR correctly\n");
    return 1;
}

TEST(ALR_sets_carry_from_bit0) {
    setup_test();

    // Set up: A = 0xFF, ALR #$FF -> A = 0x7F, C = 1 (bit 0 was 1)
    REG.A = 0xFF;
    REG.P = 0x00;

    write_byte(0x0400, 0x4B);  // ALR immediate
    write_byte(0x0401, 0xFF);  // Operand

    REG.PC = 0x0400;
    cpu_step();

    ASSERT(REG.A == 0x7F, "ALR: A should be 0x7F");
    ASSERT(GET_FLAG(FLAG_C), "ALR: Carry should be set (bit 0 was 1)");

    printf("PASS: ALR sets carry from bit 0\n");
    return 1;
}


// Test 3: ARR - AND then ROR (0x6B)
// Operation: A = (A AND immediate) ROR 1
// Complex carry/overflow behavior

TEST(ARR_and_then_rotate_right) {
    setup_test();

    // Set up: A = 0xFF, C = 0, ARR #$FF -> A = 0x7F, special flags
    REG.A = 0xFF;
    REG.P = 0x00;
    CLR_FLAG(FLAG_C);

    write_byte(0x0400, 0x6B);  // ARR immediate
    write_byte(0x0401, 0xFF);  // Operand

    REG.PC = 0x0400;
    cpu_step();

    ASSERT(REG.A == 0x7F, "ARR: A should be 0x7F (0xFF rotated right with C=0)");
    ASSERT(!GET_FLAG(FLAG_N), "ARR: Negative flag should be clear");

    printf("PASS: ARR performs AND then ROR with carry\n");
    return 1;
}

TEST(ARR_uses_carry_in_rotation) {
    setup_test();

    // Set up: A = 0xFF, C = 1, ARR #$FF -> A = 0xFF (rotated with carry)
    REG.A = 0xFF;
    REG.P = 0x00;
    SET_FLAG(FLAG_C);

    write_byte(0x0400, 0x6B);  // ARR immediate
    write_byte(0x0401, 0xFF);  // Operand

    REG.PC = 0x0400;
    cpu_step();

    ASSERT(REG.A == 0xFF, "ARR: A should be 0xFF (rotated right with C=1)");
    ASSERT(GET_FLAG(FLAG_N), "ARR: Negative flag should be set");

    printf("PASS: ARR uses carry in rotation\n");
    return 1;
}


// Test 4: ALT_SBC - Alternate SBC encoding (0xEB)
// Should behave identically to regular SBC (0xE9)

TEST(ALT_SBC_identical_to_regular_SBC) {
    setup_test();

    // Test: A = 0x50, SBC #$30 with C=1 -> A = 0x20
    REG.A = 0x50;
    REG.P = 0x00;
    SET_FLAG(FLAG_C);  // Carry = 1 means no borrow
    CLR_FLAG(FLAG_D);  // Binary mode

    write_byte(0x0400, 0xEB);  // ALT_SBC (undocumented)
    write_byte(0x0401, 0x30);  // Operand

    REG.PC = 0x0400;
    cpu_step();

    ASSERT(REG.A == 0x20, "ALT_SBC: A should be 0x20 (0x50 - 0x30)");
    ASSERT(GET_FLAG(FLAG_C), "ALT_SBC: Carry should be set (no borrow)");
    ASSERT(!GET_FLAG(FLAG_N), "ALT_SBC: Negative flag should be clear");
    ASSERT(!GET_FLAG(FLAG_Z), "ALT_SBC: Zero flag should be clear");

    printf("PASS: ALT_SBC behaves identically to regular SBC\n");
    return 1;
}

TEST(ALT_SBC_with_borrow) {
    setup_test();

    // Test: A = 0x50, SBC #$70 with C=1 -> A = 0xE0 (borrow occurs)
    REG.A = 0x50;
    REG.P = 0x00;
    SET_FLAG(FLAG_C);
    CLR_FLAG(FLAG_D);

    write_byte(0x0400, 0xEB);  // ALT_SBC
    write_byte(0x0401, 0x70);  // Operand

    REG.PC = 0x0400;
    cpu_step();

    ASSERT(REG.A == 0xE0, "ALT_SBC: A should be 0xE0 (underflow)");
    ASSERT(!GET_FLAG(FLAG_C), "ALT_SBC: Carry should be clear (borrow occurred)");
    ASSERT(GET_FLAG(FLAG_N), "ALT_SBC: Negative flag should be set");

    printf("PASS: ALT_SBC handles borrow correctly\n");
    return 1;
}


// Test 5: KIL - Freeze/Halt CPU (0x02, 0x12, ..., 0xF2)
// Should halt the CPU until reset

TEST(KIL_halts_cpu) {
    setup_test();

    // Execute KIL instruction
    write_byte(0x0400, 0x02);  // KIL opcode
    write_byte(0x0401, 0xEA);  // NOP (should never execute)

    REG.PC = 0x0400;

    printf("Executing KIL at $%04X...\n", REG.PC);
    cpu_step();

    // CPU should be stopped
    ASSERT(cpu_is_stopped(), "KIL: CPU should be stopped after KIL");

    printf("PASS: KIL halts CPU correctly\n");
    return 1;
}

TEST(KIL_multiple_opcodes) {
    setup_test();

    // Test a few different KIL opcodes
    MEM_WORD kil_opcodes[] = {0x02, 0x12, 0x22, 0x32, 0x42, 0x52,
                               0x62, 0x72, 0x92, 0xB2, 0xD2, 0xF2};

    for (int i = 0; i < 12; i++) {
        setup_test();

        write_byte(0x0400, kil_opcodes[i]);
        REG.PC = 0x0400;

        cpu_step();

        if (!cpu_is_stopped()) {
            printf("FAIL: KIL opcode 0x%02X did not halt CPU\n", kil_opcodes[i]);
            return 0;
        }
    }

    printf("PASS: All 12 KIL opcodes halt CPU\n");
    return 1;
}


// Main test runner

int main(void) {
    int passed = 0;
    int total = 0;

    printf("=== Undocumented NMOS 6502 Opcodes Test Suite ===\n\n");

    printf("--- ANC Tests (AND with Carry) ---\n");
    total++; if (test_ANC_sets_carry_from_bit7()) passed++;
    total++; if (test_ANC_clears_carry_when_bit7_clear()) passed++;
    printf("\n");

    printf("--- ALR Tests (AND then LSR) ---\n");
    total++; if (test_ALR_and_then_shift_right()) passed++;
    total++; if (test_ALR_sets_carry_from_bit0()) passed++;
    printf("\n");

    printf("--- ARR Tests (AND then ROR) ---\n");
    total++; if (test_ARR_and_then_rotate_right()) passed++;
    total++; if (test_ARR_uses_carry_in_rotation()) passed++;
    printf("\n");

    printf("--- ALT_SBC Tests (Alternate SBC) ---\n");
    total++; if (test_ALT_SBC_identical_to_regular_SBC()) passed++;
    total++; if (test_ALT_SBC_with_borrow()) passed++;
    printf("\n");

    printf("--- KIL Tests (CPU Halt) ---\n");
    total++; if (test_KIL_halts_cpu()) passed++;
    total++; if (test_KIL_multiple_opcodes()) passed++;
    printf("\n");

    printf("=== Test Results ===\n");
    printf("Passed: %d/%d\n", passed, total);

    if (passed == total) {
        printf("\nAll undocumented opcode tests passed!\n");
        return 0;
    } else {
        printf("\nSome tests failed.\n");
        return 1;
    }
}
