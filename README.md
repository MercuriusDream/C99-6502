# C99-6502

[Korean / 한국어](./README_KO.md)

<img width="3000" height="1000" alt="image" src="https://github.com/user-attachments/assets/93fb6303-551e-42a6-aa75-63211b8c0d91" />

The Systems Software isn't, indeed, that sophomore-friendly. So I decided to build an emulator of a well-known microprocessor, to barely understand the course.

## Introduction

<img width="1031" height="451" alt="image" src="https://github.com/user-attachments/assets/7dde66e5-c826-4ad2-b063-2c73f3932f6c" />

*TL;DR: C99-6502 is a MOS 6502 emulator, written in ISO/IEC 9899:1999 compliant C, which supports the emulation of both NMOS and CMOS variants, with some convenient features for POSIX environments.*

This project is a cycle-accurate MOS 6502 emulator written in C99 that faithfully reproduces the behavior of the original NMOS 6502, including its documented quirks and timing characteristics.

The emulator supports the complete instruction set with stable undocumented opcodes and handles cycle counting with page-crossing penalties and branch timing. Hardware-specific behaviors like the indirect `JMP ($xxFF)` wrapping bug, zero-page address wrapping, and NMOS decimal mode flag semantics are accurately implemented. The codebase is organized into modular components covering the bus interface, region-based memory management, CPU core, addressing modes, instruction dispatch, stack operations, and execution tracing.

Memory configuration uses a region-based system where the address space is divided into separate RAM, ROM, and I/O regions. The default configuration allocates 32KiB RAM ($0000-$7FFF) and 32KiB ROM ($8000-$FFFF), mirroring the memory layout of many classic 6502 systems. ROM regions are automatically write-protected, and I/O regions support custom read/write handlers for device emulation. This flexible architecture enables accurate emulation of different systems *(e.g. NES, Apple II, Commodore 64)* by configuring appropriate memory maps for each platform.

The emulator supports both NMOS 6502 and CMOS 65C02 CPU variants. The variant can be selected via command line option (defaults to NMOS 6502). Key differences between variants include BCD flag behavior, the JMP indirect bug fix in 65C02, and instruction set additions in 65C02.

The CMOS 65C02 implementation includes all new instructions: `BRA` (Branch Always), `PHX`/`PHY` (Push X/Y), `PLX`/`PLY` (Pull X/Y), `STZ` (Store Zero), `TRB`/`TSB` (Test and Reset/Set Bits), `WAI` (Wait for Interrupt), and `STP` (Stop Processor), including all the Rockwell/WDC 65C02 bit manipulation instructions: `RMB0-7` (Reset Memory Bit), `SMB0-7` (Set Memory Bit), `BBR0-7` (Branch on Bit Reset), and `BBS0-7` (Branch on Bit Set).

Furthermore, although these components are not part of the ISO/IEC 9899:1999 standard, this emulator includes optional nonstandard features for user convenience, such as a Text User Interface (TUI) based CPU visualization monitor, built on the `ncurses` library, available in POSIX-compliant environments, and a Python 3 based system for building complex test ROMs.

### Quick Start

```bash
# Full build (POSIX environment assumed)
make

# ISO/IEC 9899:1999 compliant build
make main

# Run the minimal example ROM
make run

# Run with trace enabled
make run-trace
```

## Requirements

### Mandatory

| Requirement               | Description                                             |
| ------------------------- | ------------------------------------------------------- |
| An ISO/IEC 9899:1999 compliant compiler | *Examples include GCC and Clang* |

### Optional

| Requirement               | Description                                             |
| ------------------------- | ------------------------------------------------------- |
| make or its equivalent    | Automated build tool                                    |
| ncurses                   | Framework for TUI monitor                               |
| POSIX.1-2008 compliant environment | Precise timing than that of a non-POSIX compliant environment via `clock_gettime` |
| Python 3                  | Build tool for example ROMs                             |

*Note: ncurses is usually included within POSIX compliant environments.*

## Building

```bash
# Build the entire emulator suite (equal to make posix)
make

# Build the entire suite with POSIX environment support
make posix

# Build the C99 standard compliant main emulator
make main

# Build the TUI monitor
make monitor

# Build the example ROM
make rom

# Run the example ROM
make run

# Run the example ROM with instruction-by-instruction trace
make run-trace

# Build and run verification tests
make verify

# Clean artifacts
make clean
```

*Note: GNU make or its equivalent is required to automatically build the emulator.*

## Execute

### Argument Options

| Option               | Description                                             |
| -------------------- | ------------------------------------------------------- |
| `-f`, `--file`       | Binary file to load                                     |
| `-a`, `--address`    | Load address in hex (e.g., `8000`, `C000`)              |
| `-c`, `--cpu`        | CPU variant *(`6502`, `nmos`, `65c02`, `cmos`, defaults to `nmos`)* |
| `-t`, `--trace`      | Print instruction-by-instruction trace                  |
| `-m`, `--monitor`    | Launch interactive TUI monitor *(ncurses required)*      |
| `-r`, `--ram-start`  | RAM start address *(hexadecimal, defaults to `0000`)*           |
| `-R`, `--ram-size`   | RAM size *(in bytes, defaults to `32768`)*                 |
| `-s`, `--rom-start`  | ROM start address *(hexadecimal, defaults to `8000`)*           |
| `-S`, `--rom-size`   | ROM size *(in bytes, defaults to `32768`)*                 |

#### Examples

```bash
# Load a binary at address $8000
bin/mos6502 -f program.bin -a 8000

# Enable instruction-by-instruction trace
bin/mos6502 -f program.bin -a C000 -t

# Use CMOS 65C02 variant
bin/mos6502 -f program.bin -a 8000 -c 65c02

# Launch interactive TUI monitor (requires ncurses)
bin/mos6502 -f program.bin -a 8000 -m

# Configure custom memory layout (16KiB RAM + 16KiB ROM)
bin/mos6502 -r 0x0000 -R 16384 -s 0x4000 -S 16384

# Combine multiple options
bin/mos6502 -f program.bin -a 8000 -t -c 65c02 -r 0x0 -R 32768
```

### TUI Monitor

*TL;DR: This emulator provides an interactive text-based monitor with variants of features, to support the workloads such as visualization, debugging, or analysis.*

The emulator includes an interactive TUI (Text User Interface) monitor for real-time visualization and debugging. Launch it with the `-m` or `--monitor` flag.

*Note: ncurses is required to use the TUI monitor.*

#### Features

| Feature | Description |
| --- | --- |
| State monitoring | Registers and flags made visible |
| Live disassembly | Live viewing of upcoming instructions |
| Execution history | Recently executed instructions |
| Performance analysis | Cycle counts and frame timing's analysis |
| Memory visualization | Hexdump with ASCII representations |
| Live debugging | Step-by-step debugging including breakpoints, watchpoints, and hardware-based states |

#### Controls

| Key | Action |
| --- | ------ |
| `S` | Step - Execute one instruction |
| `C` | Continue - Resume execution |
| `B` | Break - Pause execution |
| `R` | Reset - Reset CPU state |
| `Q` | Quit - Exit monitor |

#### Display Panels

The monitor is organized into six main panels:

| Panel | Description |
| --- | --- |
| Status | CPU state (running/stopped), speed, cycle count, uptime, memory configuration |
| Registers | `PC`, `SP`, `A`, `X`, `Y`, `P` with flag breakdown (`N` `V` - `B` `D` `I` `Z` `C`) |
| Disassembly | Disassemblied instructions following from the program counter |
| Memory | Hexdump of memory regions with ASCII representation |
| Instructions | Execution history with register states |
| Performance | Real-time speed metrics (current, average, 1% low, 0.1% low) |

*Note: The TUI requires a terminal size of at least 80×19 characters.*

## Architecture

### Overview

*TL;DR: The architecture is modular, therefore each subsystem will handle their own distinct part of CPU behavior.*

The emulator is structured around several core modules. The CPU module (`cpu.c/.h`) implements the fetch/decode/execute loop with cycle accounting and interrupt handling (IRQ/NMI/Reset). Memory access is abstracted through a bus interface (`bus.c/.h`) using read/write function pointers, allowing different devices to be mapped into the address space. The memory module (`memory.c/.h`) implements a region-based system where the address space can be divided into RAM, ROM, and I/O regions. Each region can have different properties: RAM is readable and writable, ROM is read-only (write-protected), and I/O regions can have custom handlers for device emulation.

All 6502 addressing modes are implemented in `addressing.c/.h` with proper page-crossing detection. The instruction system uses a 256-entry dispatch table with metadata, where individual handlers implement operation semantics and timing. Stack operations (`stack.c/.h`) cover the $0100–$01FF range with both 8-bit and 16-bit push/pop support. Additional utilities handle execution tracing and binary loading.

### Memory

#### Memory Map

The default configuration uses a 32KiB RAM / 32KiB ROM split:

|    Range    | Type | Purpose                                    |
| ----------- | ---- | ------------------------------------------ |
| $0000–$00FF | RAM  | Zero Page                                  |
| $0100–$01FF | RAM  | Stack                                      |
| $0200–$7FFF | RAM  | General RAM                                |
| $8000–$FFF9 | ROM  | Program code and data                      |
| $FFFA–$FFFB | ROM  | NMI vector                                 |
| $FFFC–$FFFD | ROM  | Reset vector                               |
| $FFFE–$FFFF | ROM  | IRQ/BRK vector                             |

This memory layout is configurable both through command-line options (`--ram-start`, `--ram-size`, `--rom-start`, `--rom-size`) and the region-based API, which means the emulator can mimic different 6502-based systems.

#### Configure

The emulator uses a region-based memory system that allows flexible configuration:

```c
// Clear any existing regions
mem_region_clear();

// Add 32KiB RAM at $0000-$7FFF
mem_region_add_ram(0x0000, 0x8000);

// Add 32KiB ROM at $8000-$FFFF
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

### Accuracy

*TL;DR: every CPU instruction is emulated at the same number of clock cycles as real hardware, including page-crossing delays and branching penalties. Also, those hardware-based quirks are emulated, too.*

Timing follows the original hardware specifications with base cycle counts per opcode. Additional cycles are added for page crossings on indexed reads (ABSX/ABSY/INDY), taken branches, and page boundary crossings during branch execution.

Instruction timing and NMOS-specific hardware oddities have been matched cycle-for-cycle with the original microprocessor, such as Zero-page indexed addressing modes wrapping at the `$00FF` boundary, or `(IND,X)` pointer calculations wrapping within the zero page. See the CPU Variant Differences section for details on NMOS vs CMOS behavioral differences.

### CPU Variant

*TL;DR: NMOS and CMOS 65C02 behave differently in subtle but important ways. this emulator models both.*

The emulator supports both NMOS 6502 and CMOS 65C02 modes with the following behavioral differences:

| Aspect | NMOS 6502 | CMOS 65C02 |
| ------ | --------- | ----------- |
| BCD (Decimal) Mode Flags | `N` and `Z` flags reflect the binary result before BCD adjustment; `V` is computed from the binary operation. | `N` and `Z` flags reflect the adjusted decimal result; `V` is computed from the binary operation. |
| Indirect `JMP` at `$xxFF` | `JMP ($xxFF)` wraps within the page, which, reads high byte from `$xx00` of the same page. | Page-crossing bug is fixed; `JMP ($xxFF)` reads the high byte from the next page. |
| Instruction Set | Base 6502 instruction set (56 official opcodes), including supported NMOS undocumented opcodes (`LAX`, `SAX`, `DCP`, `ISC`, `SLO`, `RLA`, `SRE`, `RRA`). | Base 6502 instruction set plus 10 new CMOS instructions: `BRA` (Branch Always), `PHX`/`PHY` (Push X/Y), `PLX`/`PLY` (Pull X/Y), `STZ` (Store Zero - 4 addressing modes), `TRB`/`TSB` (Test and Reset/Set Bits), `WAI` (Wait for Interrupt), `STP` (Stop Processor). Also includes 32 Rockwell/WDC bit manipulation instructions: `RMB0-7` (Reset Memory Bit), `SMB0-7` (Set Memory Bit), `BBR0-7` (Branch on Bit Reset), `BBS0-7` (Branch on Bit Set). Undocumented opcodes are treated as NOPs. |
| Variant Checking | Instructions execute without variant checks. | 65C02-specific instructions only execute when CPU is in 65C02 mode; they become NOPs in NMOS mode. |

The decimal (BCD) mode `N`/`Z` flag behavior is the most commonly encountered difference in practice. Both the carry flag (`C`) and overflow flag (`V`) behave identically between variants.

### Interrupt

The emulator includes a comprehensive interrupt controller that accurately models 6502 interrupt behavior:

#### Features

| Feature                        | Description                                                      |
|---------------------------------|------------------------------------------------------------------|
| Software Interrupt (`BRK`)        | Executes `BRK` instruction, sets `B` flag, uses `IRQ`/`BRK` vector       |
| Maskable Interrupt (`IRQ`)        | Level-triggered, respects `I` flag (interrupt disable)             |
| Non-Maskable Interrupt (`NMI`)    | Edge-triggered (falling edge), always executes, uses `NMI` vector  |
| Edge Detection                  | `NMI` triggers only on 1→0 transition (falling edge)               |
| Interrupt Priority              | `NMI` has higher priority, can hijack `IRQ` sequence                 |
| Interrupt History               | Records last 32 interrupts for debugging                         |
| Statistics Tracking             | Counts total `IRQ`s, `NMI`s, `BRK`s for profiling                      |
| Hardware-Accurate Timing        | 7-cycle interrupt sequence models real 6502 hardware             |

#### Interrupt List

| Type | Trigger | Maskable | Vector | B Flag |
|------|---------|----------|--------|--------|
| `BRK` | Software | No | `$FFFE` | Set (1) |
| `IRQ` | Level | Yes (`I` flag) | `$FFFE` | Clear (0) |
| `NMI` | Edge (falling) | No | `$FFFA` | Clear (0) |

#### Usage Example

```c
#include "interrupt.h"

// Initialize interrupt controller
cpu_init();  // Automatically initializes interrupts

// Trigger IRQ (level-triggered)
interrupt_set_irq(1);  // Assert IRQ line
// CPU will service IRQ if I flag is clear

// Trigger NMI (edge-triggered)
interrupt_set_nmi(1);  // Set NMI line high
interrupt_set_nmi(0);  // Create falling edge → triggers NMI

// Check for pending interrupts
INTERRUPT_TYPE type = interrupt_poll();
if (type == INT_NMI) {
    // NMI is pending
}

// View interrupt history
interrupt_dump_history();
interrupt_dump_stats();
```

#### Running the Interrupt Test

```bash
make interrupt-test
./bin/interrupt_test
```

The interrupt test demonstrates BRK, IRQ, NMI, edge detection, priority handling, and history tracking.

### Supported Instructions

All official 6502 opcodes are implemented. When running in NMOS mode, undocumented opcodes are supported: `LAX`, `SAX`, `DCP`, `ISC`, `SLO`, `RLA`, `SRE`, `RRA`, and common NOP variants used on real NMOS parts. Highly unstable opcodes (such as `$9B`, `$9C`, `$9E`, `$9F`) are intentionally omitted due to unpredictable behavior on real hardware.

In 65C02 mode, all standard WDC 65C02 instructions are supported, including the *complete set of Rockwell/WDC bit manipulation extensions*:

`BRA`, `PHX`, `PHY`, `PLX`, `PLY`, `STZ` (4 addressing modes), `TRB`, `TSB`, `WAI`, `STP`, *`RMB0-7`, `SMB0-7`, `BBR0-7`, `BBS0-7` (32 instructions total)*

*Note: In 65C02 mode, most undocumented opcodes were officially changed to NOPs. The current implementation treats them as NOPs in both modes, which is functionally correct for 65C02 but means some NMOS-specific undocumented opcodes won't work in NMOS mode if they're unimplemented.*

## Debugging and Profiling Tools

The emulator includes a comprehensive debugging and profiling system to help analyze program execution, find bugs, or optimize code. The debugging features are provided through the `debugging.h` library. Key features include:

### Features Overview

| Feature              | Description                                                                 |
|----------------------|-----------------------------------------------------------------------------|
| Breakpoints          | Set breakpoints for execution, memory reads, writes, or access at specific addresses. |
| Watchpoints          | Track changes to specific memory locations and receive notifications.        |
| Cycle Profiling      | Measure total cycle counts and analyze per-instruction execution frequency.  |
| Hotspot Analysis     | Detect the most frequently executed code regions.                            |
| Memory Inspection    | View memory contents with hex dumps, ASCII representation, and disassembly.  |
| Register Inspection  | Examine the full CPU state, including detailed flag information.             |
| Stack Inspection     | Analyze the stack's current state and contents.                              |
| Memory Search        | Search for specific byte patterns across the entire memory space.            |

### Example Usage

```c
#include "debugging.h"

// Initialize debugger
debugger_init();

// Set breakpoints
debugger_add_breakpoint(BP_TYPE_EXEC, 0x8000, "main_loop");
debugger_add_breakpoint(BP_TYPE_WRITE, 0x0200, "output_port");

// Add watchpoints
debugger_add_watchpoint(0x0200, "counter_variable");

// Check breakpoints during execution
if (debugger_check_breakpoint(BP_TYPE_EXEC, REG.PC)) {
    debugger_dump_registers(); // Dump CPU state
}

// Perform profiling
profiler_record_instruction(pc, opcode, cycles);
profiler_dump_stats();           // Show instruction frequency
profiler_dump_hotspots(10);      // Show top 10 hotspots

// Memory inspection
debugger_hexdump(0x8000, 256);        // Hex dump with ASCII
debugger_disassemble(0x8000, 20);     // Disassemble instructions
debugger_dump_stack();                // Dump stack contents
debugger_dump_registers();            // Show registers and flags

// Memory search
MEM_WORD pattern[] = { 0xA9, 0x42 };  // LDA #$42
debugger_search_memory(0x8000, 0xFFFF, pattern, 2);

debugger_cleanup(); // Cleanup
```

### Testing the Debugger

```bash
make debug-test
./bin/debug_test
```

The debug test demonstrates all debugging features with a sample program that loops, modifies memory, and performs arithmetic operations.

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

The functional test runner (`tests/6502_functional_test/run_functional_test.c`) configures 64KiB of RAM, loads the test binary, and detects test success or failure by monitoring the program counter. Test progress is displayed as dots(`.`), with each dot representing a million cycles.

### Basic Verification Tests

The project also includes a small example ROM, with Host tools, which we call the *verification test suite*, in `tests/verify_test.c` for basic verifications:

```bash
# NMOS 6502 verification
make verify
bin/mos6502 -f tests/minimal/test.bin -a 8000 -t

# CMOS 65C02 verification
cd tests/minimal && python3 build_65c02_test.py && cd ../..
clang -std=c99 -O2 -Wall -Iinclude src/*.c tests/minimal/verify_65c02_test.c -o bin/verify_65c02_test
bin/verify_65c02_test
```

On top of the 6502 test suite, 65C02 test suite also validates all of the new CMOS instructions: `BRA`, `PHX`, `PHY`, `PLX`, `PLY`, `STZ` (all addressing modes), `TSB`, and `TRB`.

*Note: Python 3 is needed to build the 65C02 verification tests.*

## Limitations

This is a CPU-focused emulator without peripheral device implementations (PPU, APU, keyboard, etc.). The memory system provides the foundation for device emulation through configurable I/O regions with custom handlers, but no specific devices are currently implemented. A subset of highly unstable undocumented opcodes (such as `$9B`, `$9C`, `$9E`, `$9F`) is intentionally not implemented due to unpredictable behavior on real hardware.

## Coding Conventions

The codebase follows consistent naming conventions: global identifiers use ALL_CAPS (such as `REG`, `BUS`, `EA`), typedefs are prefixed with `T_` (like `T_REGISTER`), and constants or macros use ALL_CAPS (such as `FLAG_C`). Code is indented with 4 spaces, and the standard practice of placing declarations in `.h` files and definitions in `.c` files is followed throughout.

## Project Layout

```Text
.
├── src/                      # Core sources
│   ├── addressing.c
│   ├── bus.c
│   ├── cpu.c
│   ├── debugging.c           # Debugging and profiling
│   ├── instructions_handlers.c        # Opcode dispatch
│   ├── instructions_implementation.c  # Opcode handlers
│   ├── instructions_table.c           # Opcode metadata
│   ├── interrupt.c           # Interrupt controller
│   ├── loader.c
│   ├── logging.c             # Logging system
│   ├── memory.c
│   ├── stack.c
│   ├── trace.c
│   └── tui_monitor.c         # TUI monitor implementation
├── tests/                    # Test suites
│   ├── 6502_functional_test/ # Klaus Dormann functional test
│   ├── minimal/              # Minimal and 65C02 test suite
│   ├── verify_test.c         # Basic verification
│   ├── debug_test.c          # Debugger test
│   └── interrupt_test.c      # Interrupt test
├── include/                  # Header files
│   ├── addressing.h
│   ├── bus.h
│   ├── cpu.h
│   ├── debugging.h
│   ├── instructions_handlers.h
│   ├── instructions_implementation.h
│   ├── instructions_table.h
│   ├── interrupt.h
│   ├── loader.h
│   ├── logging.h
│   ├── memory.h
│   ├── stack.h
│   ├── trace.h
│   ├── tui_monitor.h         # TUI monitor header
│   └── types.h
├── bin/                      # Build outputs
├── main.c
├── Makefile
├── README.md
├── README_KO.md              # Korean version of README
└── .gitignore
```

## References

This implementation is based on the MOS Technology 6502 Programming Manual, documentation from [6502.org](http://6502.org/), and insights from the [Visual 6502](http://visual6502.org/) transistor-level simulation project.

## License

This project is being protected under the GNU Affero General Public License Version 3.0. For licensing terms, please see the [LICENSE](LICENSE).

### Open Source License

[6502_functional_test](https://github.com/klaus-dormann/6502_functional_test) and this project's related sources in the [tests/6502_functional_test](tests/6502_functional_test) folder are protected under the GNU General Public License Version 3.0. For licensing terms, please see the [README of the 6502_functional_test](tests/6502_functional_test/README.md) and [LICENSE of the 6502_functional_test](tests/6502_functional_test/LICENSE).

## Also

Thank you for reading this to the very end! I wish you a good day (˶˃ ᵕ ˂˶) .ᐟ.ᐟ

<sub>*Written by a CS Sophomore from South Korea, Powered by Methylphenidate Hydrochloride, Which requires Lisdexamfetamine Dimesylate to function, While that being illegal in South Korea. *(Which means, I suffer w/ Maladaptive Daydreaming and Executive Dysfunction.)<br>~~Someone please get me outta SK~~*</sub>
