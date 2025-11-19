# MOS 6502 Test ROM

[Korean / 한국어](./README_KO.md)

## Introduction

A comprehensive test ROM for the MOS 6502 emulator that exercises various instructions and addressing modes. The test suite includes a Python build script (`build_test_rom.py`), and a verification program (`verify_test.c`) for automated result checking.

## Building the ROM

```bash
python3 build_test_rom.py
```

This generates `test.bin` which can be loaded into the emulator.

## Running the ROM

### With the emulator (trace enabled)
```bash
./mos6502 -f test.bin -a 8000 -t
```

### With the emulator (normal execution)
```bash
./mos6502 -f test.bin -a 8000
```

### With verification
```bash
clang -std=c99 -O2 verify_test.c cpu.c bus.c memory.c stack.c \
    addressing.c instructions_implementation.c instructions_handlers.c \
    instructions_table.c loader.c trace.c -o verify_test

./verify_test
```

## Test Coverage

The ROM includes 15 tests exercising fundamental 6502 operations. Basic load and store operations are tested through `LDA`/`STA`, `LDX`/`STX`, and `LDY`/`STY` instructions. Arithmetic operations include `ADC` (addition with carry, $10 + $05 = $15) and `SBC` (subtraction with carry, $15 - $03 = $12). Logical operations cover `AND` ($0F & $03 = $03), `ORA` ($05 | $02 = $07), and `EOR` (exclusive OR, $FF ^ $0F = $F0).

Register operations test `INX` (increment X three times) and `DEY` (decrement Y twice). Control flow is verified through `BEQ` (branch if equal), `JSR`/`RTS` (subroutine call and return), and `PHA`/`PLA` (stack push and pull). Addressing modes are validated with absolute indexed (ABS,X) and zero page indexed (ZP,X) operations.

## Memory Map

```
$0200-$020E  Test result storage
$8000-$8073  Main test code
$8074-$807D  MULTIPLY subroutine
$807E-$8082  DATA array
```

## Expected Results

After execution, memory locations should contain:

| Address | Value | Test Description |
|---------|-------|------------------|
| $0200 | $42 | `LDA #$42`, `STA $0200` |
| $0201 | $10 | `LDX #$10`, `STX $0201` |
| $0202 | $20 | `LDY #$20`, `STY $0202` |
| $0203 | $15 | `ADC`: $10 + $05 |
| $0204 | $12 | `SBC`: $15 - $03 |
| $0205 | $03 | `AND`: $0F & $03 |
| $0206 | $07 | `ORA`: $05 \| $02 |
| $0207 | $F0 | `EOR`: $FF ^ $0F |
| $0208 | $03 | `INX` executed 3 times |
| $0209 | $03 | `DEY` executed 2 times from $05 |
| $020A | $AA | Branch test result |
| $020B | $06 | Subroutine: 2+2+2 |
| $020C | $55 | Stack test (`PHA`/`PLA`) |
| $020D | $44 | Indexed addressing: DATA[3] |
| $020E | $99 | Zero page indexed |

## Test Details

Arithmetic tests verify `ADC` and `SBC` with proper carry flag management. Logical operations test bitwise `AND`, `OR`, and `XOR` functionality. Control flow verification includes conditional branching with `BEQ` and the subroutine call mechanism through `JSR`/`RTS`.

The test suite exercises multiple addressing modes including immediate (`LDA #$42`), absolute (`STA $0200`), zero page (`STA $10`), absolute indexed (`LDA $807E,X`), and zero page indexed (`LDA $10,X`). Register operations cover transfers between accumulator and index registers (`TAX`, `TAY`, `TXA`), increment and decrement operations (`INX`, `INY`, `DEX`, `DEY`), and stack manipulation (`PHA`, `PLA`).

## Verification

The verification program runs all 15 tests and reports PASS for values matching expectations or FAIL for mismatches. All tests should pass on a correctly functioning emulator.