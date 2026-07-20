/*
 * Interrupt Controller Test Program
 * Real assertion-based tests for: BRK, IRQ, NMI, RTI, reset, and 65C02 D-flag.
 *
 * IMPORTANT contract (see task): cpu_irq() / cpu_nmi() only ASSERT the
 * interrupt line; the interrupt is serviced on the NEXT cpu_step(). So every
 * cpu_irq()/cpu_nmi() call below is followed by a cpu_step() before any state
 * is inspected.
 */

#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "interrupt.h"

static int failures = 0;

#define CHECK8(actual, expected, msg)                                       \
    do {                                                                   \
        MEM_WORD _a = (MEM_WORD)(actual);                                  \
        MEM_WORD _e = (MEM_WORD)(expected);                                \
        if (_a != _e) {                                                    \
            printf("FAIL: %s got $%02X expected $%02X\n", msg, _a, _e);     \
            failures++;                                                    \
        } else {                                                           \
            printf("PASS: %s ($%02X)\n", msg, _a);                          \
        }                                                                  \
    } while (0)

#define CHECK16(actual, expected, msg)                                    \
    do {                                                                  \
        MEM_TWO_WORDS _a = (MEM_TWO_WORDS)(actual);                       \
        MEM_TWO_WORDS _e = (MEM_TWO_WORDS)(expected);                     \
        if (_a != _e) {                                                   \
            printf("FAIL: %s got $%04X expected $%04X\n", msg, _a, _e);    \
            failures++;                                                   \
        } else {                                                          \
            printf("PASS: %s ($%04X)\n", msg, _a);                        \
        }                                                                 \
    } while (0)

#define CHECKFLAG(actual, expected, msg)                                  \
    do {                                                                  \
        int _a = (actual) ? 1 : 0;                                        \
        int _e = (expected) ? 1 : 0;                                      \
        if (_a != _e) {                                                   \
            printf("FAIL: %s got %d expected %d\n", msg, _a, _e);          \
            failures++;                                                   \
        } else {                                                          \
            printf("PASS: %s (%d)\n", msg, _a);                           \
        }                                                                 \
    } while (0)

/* Set up a fresh RAM+ROM address space and point the reset/IRQ/NMI vectors. */
static void setup_memory(MEM_TWO_WORDS reset_pc,
                         MEM_TWO_WORDS irq_pc,
                         MEM_TWO_WORDS nmi_pc) {
    mem_region_clear();
    mem_region_add_ram(0x0000, 0x8000);
    mem_region_add_rom(0x8000, 0x8000);
    mem_region_init();
    mem_region_set_vector(CPU_RESET_VECTOR_ADDRESS, reset_pc);
    mem_region_set_vector(IRQ_VECTOR, irq_pc);
    mem_region_set_vector(NMI_VECTOR, nmi_pc);
}

static void load(MEM_TWO_WORDS addr, const MEM_WORD* data, MEM_TWO_WORDS len) {
    mem_region_load(addr, data, len);
}

/* ------------------------------------------------------------------ */
/* Reset state test                                                     */
/* ------------------------------------------------------------------ */
static void test_reset_state(void) {
    printf("\n--- Test: Reset state ---\n");
    cpu_set_variant(CPU_VARIANT_NMOS_6502);
    setup_memory(0x8000, 0x9000, 0x9200);
    cpu_reset();

    CHECK16(REG.PC, 0x8000, "reset loads PC from reset vector");
    CHECK8(REG.S, CPU_RESET_STACK_POINTER, "reset SP=$FD");
    CHECK8(REG.P, CPU_RESET_STATUS, "reset P=$24 (U=1,I=1)");
    CHECK8(REG.A, 0x00, "reset A=0");
    CHECK8(REG.X, 0x00, "reset X=0");
    CHECK8(REG.Y, 0x00, "reset Y=0");
}

/* ------------------------------------------------------------------ */
/* BRK pushes PC+2 and P with B=1,U=1; RTI restores P and PC            */
/* ------------------------------------------------------------------ */
static void test_brk_and_rti(void) {
    printf("\n--- Test: BRK + RTI ---\n");
    cpu_set_variant(CPU_VARIANT_NMOS_6502);

    /* $8000: LDA #$11        ; sets A, leaves P=$24 (U,I set)            */
    /* $8002: BRK             ; software interrupt                       */
    /* $8003: $00 (padding)   ; skipped by BRK (PC pushed = $8004)       */
    /* $8004: LDA #$22        ; RTI should return here                    */
    MEM_WORD prog[] = {
        0xA9, 0x11,        /* LDA #$11 */
        0x00,              /* BRK       */
        0x00,              /* padding   */
        0xA9, 0x22         /* LDA #$22  */
    };
    /* $9000: BRK/IRQ handler: LDA #$99; STA $0200; RTI */
    MEM_WORD handler[] = {
        0xA9, 0x99,
        0x8D, 0x00, 0x02,
        0x40
    };
    setup_memory(0x8000, 0x9000, 0x9200);
    load(0x8000, prog, sizeof(prog));
    load(0x9000, handler, sizeof(handler));
    cpu_reset();

    cpu_step();                       /* LDA #$11 -> A=$11, P=$24 */

    cpu_step();                       /* BRK -> services immediately */

    /* BRK pushed return address $8004 (PC+2 relative to BRK opcode). */
    CHECK8(REG.S, 0xFA, "BRK: SP decremented to $FA");
    CHECK8(bus_read(0x01FD), 0x80, "BRK: pushed PCH=$80");
    CHECK8(bus_read(0x01FC), 0x04, "BRK: pushed PCL=$04 (PC+2)");
    /* Pushed P should have B=1 and U=1. P was $24, so pushed = $24|B|U = $34. */
    CHECK8(bus_read(0x01FB), 0x34, "BRK: pushed P with B=1,U=1 ($34)");
    CHECK16(REG.PC, 0x9000, "BRK: PC jumps to IRQ/BRK vector");
    CHECKFLAG(GET_FLAG(FLAG_B), 0, "BRK: B flag not set in live REG.P");

    /* Run the handler: LDA #$99, STA $0200, RTI. */
    cpu_step();                       /* LDA #$99 */
    cpu_step();                       /* STA $0200 */
    cpu_step();                       /* RTI       */

    CHECK8(bus_read(0x0200), 0x99, "BRK handler ran and wrote $0200=$99");
    CHECK8(REG.A, 0x99, "handler left A=$99");
    CHECK8(REG.S, 0xFD, "RTI: SP restored to $FD");
    /* RTI pops P and strips B and U. Pushed P was $34 -> $34 & ~(B|U) = $04. */
    CHECK8(REG.P, 0x04, "RTI: P restored with B/U stripped ($04)");
    CHECK16(REG.PC, 0x8004, "RTI: PC returns to $8004 (BRK return addr)");
}

/* ------------------------------------------------------------------ */
/* IRQ: ignored when I=1, serviced when I clears; pushes P with B=0,U=1*/
/* ------------------------------------------------------------------ */
static void test_irq_masking_and_service(void) {
    printf("\n--- Test: IRQ masking & service ---\n");
    cpu_set_variant(CPU_VARIANT_NMOS_6502);

    /* $8000: NOP                                                */
    /* $8001: NOP     (IRQ should be ignored here, I=1)          */
    /* $8002: CLI     (clear I)                                  */
    /* $8003: NOP     (loop target; IRQ serviced at this step)   */
    /* $8004: JMP $8003                                          */
    MEM_WORD prog[] = {
        0xEA,                    /* NOP    */
        0xEA,                    /* NOP    */
        0x58,                    /* CLI    */
        0xEA,                    /* NOP    */
        0x4C, 0x03, 0x80         /* JMP $8003 */
    };
    /* $9000: IRQ handler: LDA #$AA; STA $0211; RTI */
    MEM_WORD handler[] = {
        0xA9, 0xAA,
        0x8D, 0x11, 0x02,
        0x40
    };
    setup_memory(0x8000, 0x9000, 0x9200);
    load(0x8000, prog, sizeof(prog));
    load(0x9000, handler, sizeof(handler));
    cpu_reset();                    /* I=1 after reset */

    cpu_step();                     /* NOP at $8000 -> PC=$8001 */

    /* Assert IRQ line while I=1: must be ignored. */
    cpu_irq();
    cpu_step();                     /* with I=1: no service, executes NOP at $8001 */

    CHECK8(REG.S, 0xFD, "IRQ ignored (I=1): stack untouched, SP=$FD");
    CHECK16(REG.PC, 0x8002, "IRQ ignored (I=1): PC advances to $8002 (NOP executed)");

    cpu_step();                     /* CLI at $8002 -> I=0, PC=$8003 */

    /* IRQ line still asserted; next step must service it. */
    cpu_step();                     /* services IRQ */

    CHECK8(REG.S, 0xFA, "IRQ serviced: SP decremented to $FA");
    CHECK8(bus_read(0x01FD), 0x80, "IRQ: pushed PCH=$80");
    CHECK8(bus_read(0x01FC), 0x03, "IRQ: pushed PCL=$03 (return PC=$8003)");
    /* Pushed P: P was $20 after CLI (I=0,U=1), B=0,U=1 -> $20. */
    CHECK8(bus_read(0x01FB), 0x20, "IRQ: pushed P with B=0,U=1 ($20)");
    CHECK16(REG.PC, 0x9000, "IRQ serviced: PC jumps to IRQ vector");
    CHECKFLAG(GET_FLAG(FLAG_I), 1, "IRQ serviced: I flag set in REG.P");

    /* Deassert IRQ line so RTI does not immediately re-trigger. */
    interrupt_set_irq(0);

    cpu_step();                     /* handler LDA #$AA */
    cpu_step();                     /* handler STA $0211 */
    cpu_step();                     /* handler RTI */

    CHECK8(bus_read(0x0211), 0xAA, "IRQ handler ran and wrote $0211=$AA");
    CHECK8(REG.S, 0xFD, "RTI after IRQ: SP restored to $FD");
    CHECK16(REG.PC, 0x8003, "RTI after IRQ: PC returns to $8003");
}

/* ------------------------------------------------------------------ */
/* NMI fires even when I=1 (non-maskable)                              */
/* ------------------------------------------------------------------ */
static void test_nmi_unmaskable(void) {
    printf("\n--- Test: NMI fires with I=1 ---\n");
    cpu_set_variant(CPU_VARIANT_NMOS_6502);

    /* $8000: SEI     (ensure I=1)                                   */
    /* $8001: NOP     (NMI serviced at this step)                    */
    /* $8002: JMP $8001                                              */
    MEM_WORD prog[] = {
        0x78,                    /* SEI */
        0xEA,                    /* NOP */
        0x4C, 0x01, 0x80         /* JMP $8001 */
    };
    /* $9200: NMI handler: LDA #$BB; STA $0212; RTI */
    MEM_WORD handler[] = {
        0xA9, 0xBB,
        0x8D, 0x12, 0x02,
        0x40
    };
    setup_memory(0x8000, 0x9000, 0x9200);
    load(0x8000, prog, sizeof(prog));
    load(0x9200, handler, sizeof(handler));
    cpu_reset();                    /* I=1 */

    cpu_step();                     /* SEI -> I=1, PC=$8001 */
    cpu_step();                     /* NOP -> PC=$8002 */
    cpu_step();                     /* JMP $8001 -> PC=$8001 */

    /* NMI is edge-triggered and non-maskable. */
    cpu_nmi();
    cpu_step();                     /* services NMI despite I=1 */

    CHECK8(REG.S, 0xFA, "NMI: SP decremented to $FA");
    CHECK8(bus_read(0x01FD), 0x80, "NMI: pushed PCH=$80");
    CHECK8(bus_read(0x01FC), 0x01, "NMI: pushed PCL=$01 (return PC=$8001)");
    /* P was $24 (I=1,U=1); pushed P with B=0,U=1 -> $24. */
    CHECK8(bus_read(0x01FB), 0x24, "NMI: pushed P with B=0,U=1 ($24)");
    CHECK16(REG.PC, 0x9200, "NMI serviced: PC jumps to NMI vector");

    cpu_step();                     /* handler LDA #$BB */
    cpu_step();                     /* handler STA $0212 */
    cpu_step();                     /* handler RTI */

    CHECK8(bus_read(0x0212), 0xBB, "NMI handler ran and wrote $0212=$BB");
    CHECK8(REG.S, 0xFD, "RTI after NMI: SP restored to $FD");
    CHECK16(REG.PC, 0x8001, "RTI after NMI: PC returns to $8001");
}

/* ------------------------------------------------------------------ */
/* 65C02: BRK/IRQ/NMI clear the D flag                                  */
/* ------------------------------------------------------------------ */
static void test_65c02_clears_d_flag(void) {
    printf("\n--- Test: 65C02 clears D flag on interrupt ---\n");
    cpu_set_variant(CPU_VARIANT_CMOS_65C02);

    /* Shared handler used for all three: RTI only. */
    MEM_WORD handler[] = { 0x40 };  /* RTI */

    /* --- BRK --- */
    /* $8000: SED ; $8001: BRK ; $8002: padding ; $8003: NOP */
    MEM_WORD brk_prog[] = {
        0xF8,        /* SED */
        0x00,        /* BRK */
        0x00,        /* padding */
        0xEA         /* NOP */
    };
    setup_memory(0x8000, 0x9000, 0x9200);
    load(0x8000, brk_prog, sizeof(brk_prog));
    load(0x9000, handler, sizeof(handler));
    cpu_reset();
    cpu_step();                     /* SED -> D=1 */
    cpu_step();                     /* BRK -> service */
    CHECKFLAG(GET_FLAG(FLAG_D), 0, "65C02 BRK: D flag cleared after service");

    /* --- IRQ --- */
    /* $8000: SED ; $8001: CLI ; $8002: NOP (loop) ; $8003: JMP $8002 */
    MEM_WORD irq_prog[] = {
        0xF8,                    /* SED */
        0x58,                    /* CLI */
        0xEA,                    /* NOP */
        0x4C, 0x02, 0x80         /* JMP $8002 */
    };
    setup_memory(0x8000, 0x9000, 0x9200);
    load(0x8000, irq_prog, sizeof(irq_prog));
    load(0x9000, handler, sizeof(handler));
    cpu_reset();
    cpu_step();                     /* SED -> D=1 */
    cpu_step();                     /* CLI -> I=0, D=1 */
    cpu_step();                     /* NOP -> PC=$8003 */
    cpu_step();                     /* JMP $8002 -> PC=$8002 */
    cpu_irq();
    cpu_step();                     /* service IRQ */
    CHECKFLAG(GET_FLAG(FLAG_D), 0, "65C02 IRQ: D flag cleared after service");
    interrupt_set_irq(0);

    /* --- NMI --- */
    /* $8000: SED ; $8001: NOP (loop) ; $8002: JMP $8001 */
    MEM_WORD nmi_prog[] = {
        0xF8,                    /* SED */
        0xEA,                    /* NOP */
        0x4C, 0x01, 0x80         /* JMP $8001 */
    };
    setup_memory(0x8000, 0x9000, 0x9200);
    load(0x8000, nmi_prog, sizeof(nmi_prog));
    load(0x9200, handler, sizeof(handler));
    cpu_reset();
    cpu_step();                     /* SED -> D=1 */
    cpu_step();                     /* NOP -> PC=$8002 */
    cpu_step();                     /* JMP $8001 -> PC=$8001 */
    cpu_nmi();
    cpu_step();                     /* service NMI */
    CHECKFLAG(GET_FLAG(FLAG_D), 0, "65C02 NMI: D flag cleared after service");

    cpu_set_variant(CPU_VARIANT_NMOS_6502);
}

/* ------------------------------------------------------------------ */
/* Interrupt controller statistics                                      */
/* ------------------------------------------------------------------ */
static void test_interrupt_stats(void) {
    printf("\n--- Test: interrupt statistics ---\n");
    INTERRUPT_CONTROLLER* ic = interrupt_get_controller();
    CHECKFLAG(ic->total_brks >= 1, 1, "stats: at least one BRK recorded");
    CHECKFLAG(ic->total_irqs >= 1, 1, "stats: at least one IRQ recorded");
    CHECKFLAG(ic->total_nmis >= 1, 1, "stats: at least one NMI recorded");
}

int main() {
    printf("=== MOS 6502 Interrupt Controller Test ===\n\n");

    cpu_init();   /* initialises the interrupt controller */

    test_reset_state();
    test_brk_and_rti();
    test_irq_masking_and_service();
    test_nmi_unmaskable();
    test_65c02_clears_d_flag();
    test_interrupt_stats();

    printf("\n=== Interrupt Controller Test Complete ===\n");
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}