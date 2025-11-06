# C99-6502

[Korean / 한국어](./README_KO.md)

The Systems Software isn't, indeed, that sophomore-friendly. So I decided to build an emulator of a well-known microprocessor, to understand the most of lecture.

## Introduction

This project is a cycle-accurate MOS 6502 processor emulator written in C99 that faithfully reproduces the behavior of the original NMOS 6502, including its documented quirks and timing characteristics.

The emulator supports the complete instruction set with stable undocumented opcodes and handles cycle counting with page-crossing penalties and branch timing. Hardware-specific behaviors like the indirect `JMP ($xxFF)` wrapping bug, zero-page address wrapping, and NMOS decimal mode flag semantics are accurately implemented. The codebase is organized into modular components covering the bus interface, region-based memory management, CPU core, addressing modes, instruction dispatch, stack operations, and execution tracing.

Memory configuration uses a region-based system where the address space is divided into separate RAM, ROM, and I/O regions. The default configuration allocates 32KiB RAM ($0000-$7FFF) and 32KiB ROM ($8000-$FFFF), mirroring the memory layout of many classic 6502 systems. ROM regions are automatically write-protected, and I/O regions support custom read/write handlers for device emulation. This flexible architecture enables accurate emulation of different systems (NES, Apple II, Commodore 64) by configuring appropriate memory maps for each platform.

The emulator supports both NMOS 6502 and CMOS 65C02 CPU variants. The variant can be selected via command line option (defaults to NMOS 6502). Key differences between variants include BCD flag behavior, the JMP indirect bug fix in 65C02, and instruction set additions in 65C02.

The CMOS 65C02 implementation includes all new instructions: **BRA** (Branch Always), **PHX/PHY** (Push X/Y), **PLX/PLY** (Pull X/Y), **STZ** (Store Zero), **TRB/TSB** (Test and Reset/Set Bits), **WAI** (Wait for Interrupt), and **STP** (Stop Processor).

## Getting Started

### Requirements

A C99-compliant compiler (GCC or Clang) and Make are required to build the emulator.

Python 3 is also required to build the example ROM. *(Optional)*

### Building the Emulator

```bash
# Build emulator
make

# Build example ROM
make rom

# Run example
make run

# Run with trace
make run-trace

# Build and run verification tests
make verify

# Clean artifacts
make clean
```

## Usage

```bash
# Load a binary at address $8000
bin/mos6502 -f program.bin -a 8000

# Enable execution trace
bin/mos6502 -f program.bin -a C000 -t

# Use CMOS 65C02 variant
bin/mos6502 -f program.bin -a 8000 -c 65c02

# Configure custom memory layout (16KB RAM + 16KB ROM)
bin/mos6502 -r 0x0000 -R 16384 -s 0x4000 -S 16384

# Combine multiple options
bin/mos6502 -f program.bin -a 8000 -t -c 65c02 -r 0x0 -R 32768
```

**Options**

| Option               | Description                                             |
| -------------------- | ------------------------------------------------------- |
| `-f`, `--file`       | Binary file to load                                     |
| `-a`, `--address`    | Load address in hex (e.g., `8000`, `C000`)              |
| `-c`, `--cpu`        | CPU variant: `6502`, `nmos`, `65c02`, `cmos` (default: `nmos`) |
| `-t`, `--trace`      | Print instruction-by-instruction trace                  |
| `-r`, `--ram-start`  | RAM start address in hex (default: `0000`)              |
| `-R`, `--ram-size`   | RAM size in bytes (default: `32768`)                    |
| `-s`, `--rom-start`  | ROM start address in hex (default: `8000`)              |
| `-S`, `--rom-size`   | ROM size in bytes (default: `32768`)                    |

**Example (trace excerpt)**

```
Running...
8000  A9  LDA   A:00 X:00 Y:00 P:24 SP:FD
8002  8D  STA   A:42 X:00 Y:00 P:24 SP:FD
8005  E8  INX   A:42 X:00 Y:00 P:24 SP:FD
8006  C8  INY   A:42 X:01 Y:00 P:24 SP:FD
8007  00  BRK   A:42 X:01 Y:01 P:24 SP:FD
```

## Architecture Overview

*TL;DR: The architecture is modular, therefore each subsystem will handle their own distinct part of CPU behavior.*

The emulator is structured around several core modules. The CPU module (`cpu.c/.h`) implements the fetch/decode/execute loop with cycle accounting and interrupt handling (IRQ/NMI/Reset). Memory access is abstracted through a bus interface (`bus.c/.h`) using read/write function pointers, allowing different devices to be mapped into the address space. The memory module (`memory.c/.h`) implements a region-based system where the address space can be divided into RAM, ROM, and I/O regions. Each region can have different properties: RAM is readable and writable, ROM is read-only (write-protected), and I/O regions can have custom handlers for device emulation.

All 6502 addressing modes are implemented in `addressing.c/.h` with proper page-crossing detection. The instruction system uses a 256-entry dispatch table with metadata (source files use `instruments_*` naming for historical reasons), where individual handlers implement operation semantics and timing. Stack operations (`stack.c/.h`) cover the $0100–$01FF range with both 8-bit and 16-bit push/pop support. Additional utilities handle execution tracing and binary loading.

## Memory Map

The default configuration uses a 32KB RAM / 32KB ROM split:

|       Range | Type | Purpose                                    |
| ----------: | ---- | ------------------------------------------ |
| $0000–$00FF | RAM  | Zero Page                                  |
| $0100–$01FF | RAM  | Stack                                      |
| $0200–$7FFF | RAM  | General RAM                                |
| $8000–$FFF9 | ROM  | Program code and data                      |
| $FFFA–$FFFB | ROM  | NMI vector                                 |
| $FFFC–$FFFD | ROM  | Reset vector                               |
| $FFFE–$FFFF | ROM  | IRQ/BRK vector                             |

This memory layout is configurable both through command-line options (`--ram-start`, `--ram-size`, `--rom-start`, `--rom-size`) and the region-based API, which means the emulator can mimic different 6502-based systems.

### Configuring Memory Regions

The emulator uses a region-based memory system that allows flexible configuration:

```c
// Clear any existing regions
mem_region_clear();

// Add 32KB RAM at $0000-$7FFF
mem_region_add_ram(0x0000, 0x8000);

// Add 32KB ROM at $8000-$FFFF
mem_region_add_rom(0x8000, 0x8000);

// Load ROM file into memory
load_bin_region("program.bin", 0x8000);

// Set reset vector
mem_region_set_vector(0xFFFC, 0x8000);

// Initialize bus system
mem_region_init();
```

For I/O-mapped devices, use `mem_region_add_io()` with custom read/write handlers:

```c
MEM_WORD io_read(MEM_TWO_WORDS addr, void* ctx) {
    // Custom I/O read logic
    return 0x00;
}

void io_write(MEM_TWO_WORDS addr, MEM_WORD data, void* ctx) {
    // Custom I/O write logic
}

// Add I/O region at $6000-$6FFF
mem_region_add_io(0x6000, 0x1000, io_read, io_write, NULL);
```

ROM regions are automatically write-protected. Writes to ROM addresses are silently ignored, matching real hardware behavior.

## Accuracy Notes

*TL;DR: every CPU instruction is emulated at the same number of clock cycles as real hardware, including page-crossing delays and branching penalties. Also, those hardware-based quirks are emulated, too.*

Timing follows the original hardware specifications with base cycle counts per opcode. Additional cycles are added for page crossings on indexed reads (ABSX/ABSY/INDY), taken branches, and page boundary crossings during branch execution.

Instruction timing and NMOS-specific hardware oddities have been matched cycle-for-cycle with the original microprocessor. Such as Zero-page indexed addressing modes wrapping at the `$00FF` boundary, or `(IND,X)` pointer calculations wrapping within the zero page. See the CPU Variant Differences section for details on NMOS vs CMOS behavioral differences.

## CPU Variant Differences

*TL;DR: NMOS and CMOS 65C02 behave differently in subtle but important ways. this emulator models both.*

The emulator supports both NMOS 6502 and CMOS 65C02 modes with the following behavioral differences:

| Aspect | NMOS 6502 | CMOS 65C02 |
| ------ | --------- | ----------- |
| BCD (Decimal) Mode Flags | N and Z flags reflect the binary result before BCD adjustment; V is computed from the binary operation. | N and Z flags reflect the adjusted decimal result; V is computed from the binary operation. |
| Indirect `JMP` at `$xxFF` | `JMP ($xxFF)` wraps within the page, which, reads high byte from `$xx00` of the same page. | Page-crossing bug is fixed; `JMP ($xxFF)` reads the high byte from the next page. |
| Instruction Set | Base 6502 instruction set (56 official opcodes), including supported NMOS undocumented opcodes (LAX, SAX, DCP, ISC, SLO, RLA, SRE, RRA). | Base 6502 instruction set plus 10 new CMOS instructions: **BRA** (Branch Always), **PHX/PHY** (Push X/Y), **PLX/PLY** (Pull X/Y), **STZ** (Store Zero - 4 addressing modes), **TRB/TSB** (Test and Reset/Set Bits), **WAI** (Wait for Interrupt), **STP** (Stop Processor). Undocumented opcodes are treated as NOPs. |
| Variant Checking | Instructions execute without variant checks. | 65C02-specific instructions only execute when CPU is in 65C02 mode; they become NOPs in NMOS mode. |

*Note: Rockwell bit manipulation instructions (BBR, BBS, RMB, SMB) are not implemented.*

The decimal (BCD) mode N/Z flag behavior is the most commonly encountered difference in practice. Both the carry flag (C) and overflow flag (V) behave identically between variants.

## Supported Instructions

All official 6502 opcodes are implemented. When running in NMOS mode, undocumented opcodes are supported: **LAX, SAX, DCP, ISC, SLO, RLA, SRE, RRA**, and common NOP variants used on real NMOS parts. Highly unstable opcodes (such as `$9B`, `$9C`, `$9E`, `$9F`) are intentionally omitted due to unpredictable behavior on real hardware.

*Note: In 65C02 mode, most undocumented opcodes were officially changed to NOPs. The current implementation treats them as NOPs in both modes, which is functionally correct for 65C02 but means some NMOS-specific undocumented opcodes won't work in NMOS mode if they're unimplemented.*

## Testing

The project includes multiple testing approaches to ensure correctness:

### Klaus Dormann Functional Tests

The emulator passes the infamous Klaus Dormann 6502 functional test suite, which validates ALL official opcodes, addressing modes, flag behavior, and edge cases. Approx. 30.6M cycles are expected before the pass.

```bash
# Download the test suite (only needed once)
make download-functional-test

# Build and run the functional test
make functional-test
bin/functional_test

# Test with CMOS 65C02 variant
bin/functional_test -c 65c02
```

The functional test runner (`tests/6502_functional_test/run_functional_test.c`) configures 64KB of RAM, loads the test binary, and detects test success or failure by monitoring the program counter. Test progress is displayed as dots(`.`), with each dot representing a million cycles.

### Basic Verification Tests

The project also includes a small example ROM, with Host tools, which we call the *verification test suite* in `tests/verify_test.c` for basic verifications:

```bash
# NMOS 6502 verification
make verify
bin/mos6502 -f tests/minimal/test.bin -a 8000 -t

# Build and run 65C02 verification tests
cd tests/minimal && python3 build_65c02_test.py && cd ../..
clang -std=c17 -O2 -Wall -Iinclude src/*.c tests/minimal/verify_65c02_test.c -o bin/verify_65c02_test
bin/verify_65c02_test
```

On top of the 6502 test suite, 65C02 test suite also validates all of the new CMOS instructions: BRA, PHX, PHY, PLX, PLY, STZ (all addressing modes), TSB, and TRB.

## Limitations

This is a CPU-focused emulator without peripheral device implementations (PPU, APU, keyboard, etc.). The memory system provides the foundation for device emulation through configurable I/O regions with custom handlers, but no specific devices are currently implemented. A subset of highly unstable undocumented opcodes (such as `$9B`, `$9C`, `$9E`, `$9F`) is intentionally not implemented due to unpredictable behavior on real hardware.

## Coding Conventions

The codebase follows consistent naming conventions: global identifiers use ALL_CAPS (such as `REG`, `BUS`, `EA`), typedefs are prefixed with `T_` (like `T_REGISTER`), and constants or macros use ALL_CAPS (such as `FLAG_C`). Code is indented with 4 spaces, and the standard practice of placing declarations in `.h` files and definitions in `.c` files is followed throughout.

## Project Layout

```
.
├── src/                  # Core sources
│   ├── addressing.c
│   ├── bus.c
│   ├── cpu.c
│   ├── instruments_handlers.c          # opcode dispatch
│   ├── instruments_implementation.c    # opcode handlers
│   ├── instruments_table.c             # metadata
│   ├── loader.c
│   ├── memory.c
│   ├── stack.c
│   └── trace.c
├── tests/                # Test
│   ├── 6502_functional_test/   # Klaus Dormann functional test
│   └── minimal/          # Minimal test suite
├── include/              # Headers
├── bin/                  # Build outputs
├── main.c
├── Makefile
├── README.md
├── README_KO.md           # Korean version of README
└── .gitignore
```

## References

This implementation is based on the MOS Technology 6502 Programming Manual, documentation from [6502.org](http://6502.org/), and insights from the [Visual 6502](http://visual6502.org/) transistor-level simulation project.

## License

See the [LICENSE](LICENSE) file for licensing terms.

### Open Source License

6502_functional_test, Made by Klaus Dormann, and this project's related sources are protected under the GNU GPL Version 3 License. For licensing terms, Please see the [README of the 6502_functional_test](tests/6502_functional_test/README.md).

## Also

Thank you for reading this to the very end! I wish you a good day (˶˃ ᵕ ˂˶) .ᐟ.ᐟ

<sub>*Written by a CS Sophomore from South Korea, Powered by Methylphenidate Hydrochloride, Which requires Lisdexamfetamine Dimesylate to function, While that being illegal in South Korea. *(Which means, I suffer w/ Maladaptive Daydreaming and Executive Dysfunction.)<br>~~Someone please get me outta SK~~*</sub>