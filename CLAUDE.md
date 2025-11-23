# CLAUDE.md - AI Assistant Guide for C99-6502

This document provides comprehensive guidance for AI assistants working with the C99-6502 codebase, a cycle-accurate MOS 6502/65C02 emulator written in C99.

## Project Overview

**C99-6502** is a cycle-accurate MOS 6502 microprocessor emulator that faithfully reproduces both NMOS 6502 and CMOS 65C02 variants, including their documented quirks, timing characteristics, and hardware-specific behaviors. The project was created as an educational tool to understand systems software and computer architecture.

### Key Facts
- **Language**: C99 (strictly compliant)
- **Compiler**: Clang (primary), GCC (supported)
- **Lines of Code**: ~3,800 lines
- **Architecture**: Modular, component-based design
- **Test Coverage**: Klaus Dormann functional tests (30.6M cycles), custom verification suites
- **Special Features**: TUI monitor, interrupt controller, debugger/profiler, region-based memory system

## Codebase Structure

```
C99-6502/
├── src/                          # Core implementation (~2,500 LOC)
│   ├── addressing.c              # 6502 addressing mode implementations
│   ├── bus.c                     # Bus interface abstraction
│   ├── cpu.c                     # CPU core (fetch/decode/execute)
│   ├── debugger.c                # Debugging and profiling tools
│   ├── instructions_handlers.c   # Opcode dispatch table
│   ├── instructions_implementation.c  # Individual instruction handlers
│   ├── instructions_table.c      # Instruction metadata (mnemonics, modes)
│   ├── interrupt.c               # IRQ/NMI/BRK interrupt controller
│   ├── loader.c                  # Binary file loader
│   ├── logging.c                 # Logging subsystem
│   ├── memory.c                  # Region-based memory manager
│   ├── stack.c                   # Stack operations ($0100-$01FF)
│   ├── trace.c                   # Execution trace formatter
│   └── tui_monitor.c             # Interactive TUI monitor (ncurses)
├── include/                      # Public headers (~1,300 LOC)
│   ├── types.h                   # Type definitions, constants, enums
│   ├── cpu.h                     # CPU interface
│   ├── memory.h                  # Memory region API
│   ├── debugger.h                # Debugging API
│   ├── interrupt.h               # Interrupt controller API
│   ├── tui_monitor.h             # TUI monitor interface
│   └── [other headers]           # Matching .c files
├── tests/                        # Test suites
│   ├── 6502_functional_test/     # Klaus Dormann tests
│   ├── minimal/                  # Basic verification tests
│   ├── debug_test.c              # Debugger feature tests
│   ├── interrupt_test.c          # Interrupt behavior tests
│   └── undocumented_test.c       # NMOS undocumented opcode tests
├── main.c                        # CLI entry point
├── Makefile                      # Build system
└── README.md                     # User documentation

Build outputs: bin/ (created at build time, in .gitignore)
```

## Coding Conventions

### Naming Conventions

**CRITICAL**: This codebase has strict, non-negotiable naming conventions. Follow them precisely:

1. **Global Variables**: `ALL_CAPS`
   - `REG` (CPU registers struct)
   - `BUS` (bus interface struct)
   - `EA` (effective address)
   - `OPCODE` (current opcode)
   - `CYCLES` (cycle counter)

2. **Type Definitions**: `T_` prefix
   - `T_REGISTER` (register struct type)
   - `T_BUS` (bus struct type)

3. **Enums and Constants**: `ALL_CAPS` with descriptive prefixes
   - `FLAG_C`, `FLAG_Z`, `FLAG_N`, `FLAG_V` (status flags)
   - `CPU_VARIANT_NMOS_6502`, `CPU_VARIANT_CMOS_65C02`
   - `MEM_REGION_RAM`, `MEM_REGION_ROM`, `MEM_REGION_IO`
   - `ADDR_ZP`, `ADDR_ABSX`, `ADDR_INDY` (addressing modes)

4. **Macros**: `ALL_CAPS`
   - `SET_FLAG(b)`, `CLR_FLAG(b)`, `GET_FLAG(b)`
   - `PHY_MEM_SIZE`, `CPU_STACK_BASE`

5. **Functions**: `lowercase_with_underscores`
   - `cpu_init()`, `cpu_step()`, `cpu_reset()`
   - `mem_region_add_ram()`, `mem_region_load()`
   - `interrupt_set_irq()`, `debugger_add_breakpoint()`

6. **Function Parameters**: `ALL_CAPS` (matches global style)
   - `void bus_write(MEM_TWO_WORDS ADDR, MEM_WORD DATA)`
   - `int mem_region_add_ram(MEM_TWO_WORDS START, MEM_TWO_WORDS SIZE)`

7. **Local Variables**: `lowercase` or `snake_case` (flexible, but be consistent within function)

### Code Style

- **Indentation**: 4 spaces (NO tabs)
- **Braces**: K&R style (opening brace on same line for functions, structs, if/while/for)
- **Line Length**: Generally <100 characters, but not strictly enforced
- **Comments**:
  - English for public APIs and complex logic
  - Korean (한국어) comments appear in some implementation details (author is Korean CS student)
  - Do NOT remove or modify existing Korean comments
  - When adding new comments, prefer English for consistency with public documentation

### File Organization

- **Headers (.h)**: Declarations only
  - Include guards: `#ifndef FILENAME_H` / `#define FILENAME_H` / `#endif`
  - Public function prototypes
  - Type definitions
  - Extern declarations for global variables

- **Sources (.c)**: Definitions and implementations
  - Include corresponding header first
  - Static functions for internal helpers
  - Global variables defined here (declared extern in headers)

## Architecture Deep Dive

### CPU Core (`cpu.c`, `cpu.h`)

The CPU implements a classic fetch-decode-execute cycle:

1. **Fetch**: Read opcode from memory at `REG.PC`
2. **Decode**: Look up instruction metadata in `INST_TABLE[]`
3. **Execute**: Dispatch to handler function via `INSTR_HANDLERS[]` table
4. **Timing**: Track cycles with `CYCLES` variable

Key functions:
- `cpu_init()`: Initialize CPU state, interrupt controller
- `cpu_reset()`: Reset CPU to power-on state (PC from $FFFC, SP=$FD, P=$24)
- `cpu_step()`: Execute one instruction
- `cpu_run(MEM_TWO_WORDS MAX_CYCLES)`: Run until cycle limit
- `cpu_set_variant(CPU_VARIANT)`: Switch between NMOS/CMOS mode
- `fetch8()`, `fetch16()`: Read immediate operands
- `set_zn(MEM_WORD V)`: Update Zero and Negative flags

Global state:
```c
extern T_REGISTER REG;  // PC, A, X, Y, S, P
extern MEM_WORD OPCODE;
extern MEM_TWO_WORDS CYCLES;
extern MEM_TWO_WORDS EA;  // Effective address (computed by addressing modes)
```

### Memory System (`memory.c`, `memory.h`)

**Region-based architecture**: Address space ($0000-$FFFF) divided into configurable regions.

Region types:
- `MEM_REGION_RAM`: Read/write memory
- `MEM_REGION_ROM`: Read-only memory (writes silently ignored)
- `MEM_REGION_IO`: Custom read/write handlers for device emulation

API workflow:
```c
mem_region_clear();                    // Clear all regions
mem_region_add_ram(0x0000, 0x8000);    // 32KB RAM at $0000-$7FFF
mem_region_add_rom(0x8000, 0x8000);    // 32KB ROM at $8000-$FFFF
mem_region_load(0x8000, binary_data, size);  // Load program into ROM
mem_region_set_vector(0xFFFC, 0x8000); // Set reset vector
mem_region_init();                     // Initialize bus interface
```

**IMPORTANT**:
- Always call `mem_region_init()` after configuring regions (it sets up the bus)
- Maximum 16 regions (`MAX_MEM_REGIONS`)
- Regions must not overlap
- ROM regions are write-protected automatically

### Bus Interface (`bus.c`, `bus.h`)

Abstraction layer between CPU and memory:

```c
extern T_BUS BUS;

MEM_WORD data = BUS.READ(addr, BUS.CTX);
BUS.WRITE(addr, data, BUS.CTX);
```

The memory module configures the bus to route reads/writes to appropriate regions.

### Addressing Modes (`addressing.c`, `addressing.h`)

All 6502 addressing modes implemented with page-crossing detection:

- `ADDR_IMM`: Immediate (`#$42`)
- `ADDR_ZP`: Zero page (`$12`)
- `ADDR_ZPX`, `ADDR_ZPY`: Zero page indexed (`$12,X`)
- `ADDR_ABS`: Absolute (`$1234`)
- `ADDR_ABSX`, `ADDR_ABSY`: Absolute indexed (`$1234,X`)
- `ADDR_IND`: Indirect (`($1234)`)
- `ADDR_INDX`: Indexed indirect (`($12,X)`)
- `ADDR_INDY`: Indirect indexed (`($12),Y`)
- `ADDR_REL`: Relative (branches)
- `ADDR_ZPREL`: Zero page + relative (BBR/BBS for 65C02)
- `ADDR_ZP_IND`: Zero page indirect (65C02)
- `ADDR_ABS_IND_X`: Absolute indexed indirect (65C02 JMP)

Functions compute `EA` (effective address) and return page-crossing flag.

### Instruction System

**Three-table architecture**:

1. **`INST_TABLE[]`** (`instructions_table.c`): Metadata (256 entries)
   ```c
   typedef struct {
       const char CMD[4];  // Mnemonic (e.g., "LDA")
       ADDR_MODE ADDR;     // Addressing mode
       REG_TYPE SRC;       // Source register (if any)
       REG_TYPE DEST;      // Destination register (if any)
   } INST;
   ```

2. **`INSTR_HANDLERS[]`** (`instructions_handlers.c`): Function pointers (256 entries)
   ```c
   typedef void (*instr_fn)(void);
   extern const instr_fn INSTR_HANDLERS[256];
   ```

3. **Implementation** (`instructions_implementation.c`): Handler functions
   - Each instruction implemented as `void handler_NAME(void)`
   - Read operands using addressing mode functions
   - Update registers and flags
   - Increment `REG.PC` appropriately

Example flow for `LDA #$42`:
```
1. cpu_step() fetches opcode $A9
2. Looks up INST_TABLE[0xA9] → {CMD: "LDA", ADDR: ADDR_IMM}
3. Calls INSTR_HANDLERS[0xA9] → handler_LDA_IMM()
4. Handler reads immediate value, sets A register, updates Z/N flags
5. Cycles incremented by base cost (2) + adjustments (0)
```

### Interrupt Controller (`interrupt.c`, `interrupt.h`)

Manages IRQ, NMI, and BRK interrupts with hardware-accurate behavior:

- **IRQ**: Level-triggered, maskable (via `I` flag), vector at $FFFE
- **NMI**: Edge-triggered (falling edge 1→0), non-maskable, vector at $FFFA
- **BRK**: Software interrupt, sets `B` flag, shares IRQ vector

API:
```c
interrupt_set_irq(1);     // Assert IRQ line
interrupt_set_nmi(1);     // Set NMI high
interrupt_set_nmi(0);     // Falling edge → triggers NMI
INTERRUPT_TYPE type = interrupt_poll();  // Check for pending interrupts
```

History tracking:
```c
interrupt_dump_history();  // Show last 32 interrupts
interrupt_dump_stats();    // Show counts (IRQ, NMI, BRK)
```

### Debugger/Profiler (`debugger.c`, `debugger.h`)

Comprehensive debugging tools:

**Breakpoints**:
```c
debugger_add_breakpoint(BP_TYPE_EXEC, 0x8000, "main_loop");
debugger_add_breakpoint(BP_TYPE_WRITE, 0x0200, "output_port");
if (debugger_check_breakpoint(BP_TYPE_EXEC, REG.PC)) { /* handle */ }
```

**Watchpoints** (track memory changes):
```c
debugger_add_watchpoint(0x0200, "counter");
```

**Profiling**:
```c
profiler_record_instruction(pc, opcode, cycles);
profiler_dump_stats();       // Instruction frequency
profiler_dump_hotspots(10);  // Top 10 most executed addresses
```

**Inspection**:
```c
debugger_hexdump(0x8000, 256);       // Hex dump with ASCII
debugger_disassemble(0x8000, 20);    // Disassemble 20 instructions
debugger_dump_stack();               // Stack contents
debugger_dump_registers();           // CPU state
debugger_search_memory(0x8000, 0xFFFF, pattern, len);  // Search bytes
```

### TUI Monitor (`tui_monitor.c`, `tui_monitor.h`)

Interactive ncurses-based debugger (launched with `-m` flag):

**Six panels**:
1. Status: CPU state, speed, cycle count, uptime
2. Registers: PC, SP, A, X, Y, P (with flag breakdown)
3. Disassembly: Upcoming instructions
4. Memory: Hexdump with ASCII
5. Instructions: Execution history
6. Performance: Speed metrics (current, average, 1% low, 0.1% low)

**Controls**:
- `S`: Step one instruction
- `C`: Continue execution
- `B`: Break (pause)
- `R`: Reset CPU
- `Q`: Quit

API:
```c
monitor_init();
monitor_run();    // Blocking event loop
monitor_cleanup();
```

## CPU Variants: NMOS vs CMOS

### Switching Variants

```c
cpu_set_variant(CPU_VARIANT_NMOS_6502);   // Default
cpu_set_variant(CPU_VARIANT_CMOS_65C02);  // CMOS mode
```

### Behavioral Differences

| Feature | NMOS 6502 | CMOS 65C02 |
|---------|-----------|------------|
| **BCD Flags** | N/Z reflect binary result | N/Z reflect decimal result |
| **JMP ($xxFF)** | Wraps within page (bug) | Crosses page correctly (fixed) |
| **New Instructions** | No | BRA, PHX/PHY, PLX/PLY, STZ, TRB/TSB, WAI, STP |
| **Rockwell Extensions** | No | RMB0-7, SMB0-7, BBR0-7, BBS0-7 (32 opcodes) |
| **Undocumented Opcodes** | Supported (LAX, SAX, DCP, etc.) | Treated as NOPs |

**IMPORTANT**: When modifying instruction handlers, check if behavior should differ by variant:

```c
if (cpu_get_variant() == CPU_VARIANT_CMOS_65C02) {
    // CMOS-specific behavior
} else {
    // NMOS behavior
}
```

### 65C02-Specific Instructions

All implemented in `instructions_implementation.c`:

- **BRA** (Branch Always): Unconditional relative branch
- **PHX/PHY, PLX/PLY**: Push/pull X/Y registers
- **STZ**: Store zero (4 addressing modes: ZP, ZPX, ABS, ABSX)
- **TRB/TSB**: Test and Reset/Set Bits
- **WAI**: Wait for Interrupt (sets CPU waiting state)
- **STP**: Stop Processor (halts until reset)
- **Bit manipulation** (Rockwell/WDC):
  - `RMB0-7`: Reset Memory Bit (8 opcodes)
  - `SMB0-7`: Set Memory Bit (8 opcodes)
  - `BBR0-7`: Branch on Bit Reset (8 opcodes)
  - `BBS0-7`: Branch on Bit Set (8 opcodes)

## Build System (Makefile)

### Key Targets

**Building**:
```bash
make                    # Build main emulator → bin/mos6502
make test               # Build basic verification → bin/verify_test
make functional-test    # Build Klaus Dormann test → bin/functional_test
make debug-test         # Build debugger test → bin/debug_test
make interrupt-test     # Build interrupt test → bin/interrupt_test
make verify-65c02       # Build 65C02 verification → bin/verify_65c02_test
```

**Running**:
```bash
make run                # Run with tests/minimal/test.bin
make run-trace          # Run with execution trace
make monitor            # Launch TUI monitor
make verify             # Run basic verification tests
make run-functional-test   # Run Klaus Dormann tests
```

**Utilities**:
```bash
make rom                # Build example ROM (Python required)
make download-functional-test  # Download Klaus Dormann test suite
make clean              # Remove bin/ directory
```

### Build Configuration

- **Compiler**: `CC=clang` (can change to `gcc`)
- **Flags**: `-std=c99 -O2 -Wall -Iinclude`
- **Libraries**: `-lncurses` (for TUI monitor only)
- **Source Exclusion**: `tui_monitor.c` excluded from library builds (only in main emulator)

### Adding New Test Targets

When adding a new test file (`tests/my_test.c`):

1. Add source variable:
   ```makefile
   MY_TEST_SRC=$(TEST_DIR)/my_test.c
   ```

2. Add target variable:
   ```makefile
   MY_TEST_TARGET=$(BIN_DIR)/my_test
   ```

3. Add build target:
   ```makefile
   my-test: $(MY_TEST_TARGET)

   $(MY_TEST_TARGET): $(SOURCES) $(MY_TEST_SRC) | $(BIN_DIR)
       $(CC) $(CFLAGS) $(SOURCES) $(MY_TEST_SRC) -o $(MY_TEST_TARGET)
       @echo "Built $(MY_TEST_TARGET)"
   ```

4. Add run target:
   ```makefile
   run-my-test: $(MY_TEST_TARGET)
       $(MY_TEST_TARGET)
   ```

5. Update `.PHONY`:
   ```makefile
   .PHONY: ... my-test run-my-test
   ```

## Development Workflows

### Common Development Tasks

#### 1. Adding a New Instruction

**Example**: Adding a hypothetical `XYZ` instruction

1. **Add metadata** (`src/instructions_table.c`):
   ```c
   [0xXX] = {"XYZ", ADDR_IMM, REG_NONE, REG_A},
   ```

2. **Implement handler** (`src/instructions_implementation.c`):
   ```c
   void handler_XYZ(void) {
       MEM_WORD operand = fetch8();
       // Implement logic
       REG.A = /* computation */;
       set_zn(REG.A);
   }
   ```

3. **Add to dispatch table** (`src/instructions_handlers.c`):
   ```c
   [0xXX] = handler_XYZ,
   ```

4. **Add declaration** (`include/instructions_implementation.h`):
   ```c
   void handler_XYZ(void);
   ```

5. **Test**:
   ```bash
   make
   bin/mos6502 -f test_xyz.bin -a 8000 -t
   ```

#### 2. Adding a Memory Region Type

**Example**: Adding `MEM_REGION_FRAMEBUFFER`

1. **Add enum** (`include/types.h`):
   ```c
   typedef enum {
       MEM_REGION_NONE = 0,
       MEM_REGION_RAM,
       MEM_REGION_ROM,
       MEM_REGION_IO,
       MEM_REGION_FRAMEBUFFER  // New type
   } MEM_REGION_TYPE;
   ```

2. **Add API** (`include/memory.h`):
   ```c
   int mem_region_add_framebuffer(MEM_TWO_WORDS START, MEM_TWO_WORDS SIZE, void* fb_data);
   ```

3. **Implement** (`src/memory.c`):
   ```c
   int mem_region_add_framebuffer(MEM_TWO_WORDS START, MEM_TWO_WORDS SIZE, void* fb_data) {
       // Implementation
   }
   ```

4. **Update bus handlers** (`src/memory.c`):
   ```c
   static MEM_WORD mem_region_read(MEM_TWO_WORDS ADDR, void* CTX) {
       // Handle MEM_REGION_FRAMEBUFFER reads
   }

   static void mem_region_write(MEM_TWO_WORDS ADDR, MEM_WORD DATA, void* CTX) {
       // Handle MEM_REGION_FRAMEBUFFER writes
   }
   ```

#### 3. Writing Tests

**Integration test template**:
```c
#include <stdio.h>
#include "cpu.h"
#include "memory.h"
#include "bus.h"

int main(void) {
    // Setup
    mem_region_clear();
    mem_region_add_ram(0x0000, 0x10000);
    mem_region_init();
    cpu_init();

    // Load test program
    MEM_WORD program[] = {
        0xA9, 0x42,  // LDA #$42
        0x00         // BRK
    };
    mem_region_load(0x8000, program, sizeof(program));
    mem_region_set_vector(0xFFFC, 0x8000);

    // Run
    cpu_reset();
    cpu_step();  // Execute LDA

    // Verify
    if (REG.A != 0x42) {
        printf("FAIL: Expected A=0x42, got A=0x%02X\n", REG.A);
        return 1;
    }

    printf("PASS\n");
    return 0;
}
```

**Build and run**:
```bash
clang -std=c99 -O2 -Wall -Iinclude src/*.c my_test.c -o bin/my_test
bin/my_test
```

#### 4. Running Klaus Dormann Tests

The Klaus Dormann test is the gold standard for 6502 correctness:

```bash
# One-time setup
make download-functional-test

# Build and run NMOS test
make functional-test
bin/functional_test

# Build and run CMOS test
make test-65c02
bin/test_65c02
```

**Expected output**:
```
Loaded 65494 bytes to $0000
Reset vector: $0400
Running 6502 functional test...
.............................. (dots = millions of cycles)
SUCCESS: Test passed at PC $3469 after 30655165 cycles
```

**Debugging failures**:
- If test hangs: PC is stuck in infinite loop → bug in instruction
- If test fails: Check listing file (`tests/6502_functional_test/6502_functional_test.lst`)
- Enable trace: Modify `run_functional_test.c` to enable `trace_enable()`

### Debugging Tips

1. **Use execution trace**:
   ```bash
   bin/mos6502 -f program.bin -a 8000 -t | less
   ```
   Shows: `PC OPCODE MNEMONIC A:XX X:XX Y:XX P:XX SP:XX`

2. **Use TUI monitor**:
   ```bash
   bin/mos6502 -f program.bin -a 8000 -m
   ```
   Interactive stepping, live disassembly, memory inspection

3. **Use debugger API**:
   ```c
   debugger_init();
   debugger_add_breakpoint(BP_TYPE_EXEC, 0x8000, "suspect_code");
   // ... run CPU ...
   if (debugger_check_breakpoint(BP_TYPE_EXEC, REG.PC)) {
       debugger_dump_registers();
       debugger_hexdump(0x0000, 256);
   }
   ```

4. **Check cycle counts**:
   ```c
   printf("Cycles: %u\n", CYCLES);
   ```
   Compare against expected values from datasheets

5. **Memory inspection**:
   ```c
   MEM_WORD* ptr = mem_region_get_ptr(0x8000);
   if (ptr) {
       printf("ROM contents: %02X %02X %02X...\n", ptr[0], ptr[1], ptr[2]);
   }
   ```

## Testing Strategy

### Test Hierarchy

1. **Unit tests** (`tests/minimal/verify_*.c`): Basic instruction verification
2. **Integration tests** (`tests/debug_test.c`, `tests/interrupt_test.c`): Subsystem tests
3. **Functional tests** (`tests/6502_functional_test/`): Klaus Dormann comprehensive suite
4. **Undocumented tests** (`tests/undocumented_test.c`): NMOS undocumented opcodes

### Creating ROM Test Files

Use Python assemblers in `tests/minimal/`:

```python
# tests/minimal/build_my_test.py
opcodes = [
    0xA9, 0x42,  # LDA #$42
    0x8D, 0x00, 0x02,  # STA $0200
    0x00  # BRK
]

with open('my_test.bin', 'wb') as f:
    f.write(bytes(opcodes))
```

Run:
```bash
cd tests/minimal
python3 build_my_test.py
cd ../..
bin/mos6502 -f tests/minimal/my_test.bin -a 8000 -t
```

### Test-Driven Development Workflow

1. **Write failing test**:
   ```c
   // Test new feature
   cpu_step();
   if (REG.A != expected) {
       printf("FAIL\n");
       return 1;
   }
   ```

2. **Run test** (should fail):
   ```bash
   make my-test && bin/my_test
   ```

3. **Implement feature** (in appropriate `src/*.c` file)

4. **Run test** (should pass):
   ```bash
   make my-test && bin/my_test
   ```

5. **Run full test suite**:
   ```bash
   make verify
   make run-functional-test
   ```

## Common Pitfalls and Guidelines

### CRITICAL: What NOT to Do

1. **NEVER modify existing instruction timing** without extensive testing
   - Klaus Dormann tests are cycle-sensitive
   - Timing bugs break cycle-accurate emulation

2. **NEVER use tabs for indentation** (4 spaces only)

3. **NEVER change global variable names** (`REG`, `BUS`, `EA`, `CYCLES`, `OPCODE`)
   - Entire codebase depends on these

4. **NEVER skip `mem_region_init()`** after configuring memory
   - Bus will not be initialized → crashes

5. **NEVER assume memory is writable**
   - Use `mem_region_add_ram()` for writable regions
   - ROM writes are silently ignored (this is correct behavior)

6. **NEVER implement 65C02 instructions without variant check**
   - Example:
     ```c
     void handler_STZ(void) {
         if (cpu_get_variant() != CPU_VARIANT_CMOS_65C02) {
             return;  // NOP in NMOS mode
         }
         // CMOS implementation
     }
     ```

7. **NEVER remove Korean comments** in existing code
   - Author's native language
   - May contain implementation notes not in English comments

8. **NEVER break zero-page wrapping behavior**
   - Zero-page indexed modes MUST wrap at $FF → $00
   - This is a hardware quirk, not a bug

9. **NEVER commit binaries** to git
   - `bin/` is in `.gitignore`
   - Test ROMs (`*.bin`) are generated or downloaded

### Best Practices

1. **Always check variant mode** when implementing/modifying instructions:
   ```c
   CPU_VARIANT variant = cpu_get_variant();
   if (variant == CPU_VARIANT_CMOS_65C02) {
       // CMOS-specific logic
   }
   ```

2. **Always update all three instruction tables** when adding opcodes:
   - `INST_TABLE[]` (metadata)
   - `INSTR_HANDLERS[]` (function pointer)
   - Implementation function

3. **Always use type aliases**:
   - `MEM_WORD` for 8-bit values (not `uint8_t`)
   - `MEM_TWO_WORDS` for 16-bit addresses (not `uint16_t`)
   - `SIGNED_MEM_WORD` for signed 8-bit (not `int8_t`)

4. **Always use flag macros**:
   - `SET_FLAG(FLAG_C)` (not `REG.P |= 0x01`)
   - `CLR_FLAG(FLAG_Z)` (not `REG.P &= ~0x02`)
   - `GET_FLAG(FLAG_N)` (not `REG.P & 0x80`)

5. **Always increment `CYCLES` correctly**:
   ```c
   CYCLES = CYCLE_BASE[OPCODE];  // Base cycles
   if (page_cross) CYCLES += 1;   // Page-crossing penalty
   if (branch_taken) CYCLES += 1; // Branch penalty
   ```

6. **Always validate memory operations**:
   ```c
   MEM_WORD* ptr = mem_region_get_ptr(addr);
   if (ptr == NULL) {
       // Address not mapped
       return;
   }
   ```

7. **Always test with both variants**:
   ```bash
   bin/mos6502 -f test.bin -a 8000 -c nmos -t
   bin/mos6502 -f test.bin -a 8000 -c 65c02 -t
   ```

8. **Always run functional tests** after major changes:
   ```bash
   make run-functional-test
   make run-test-65c02
   ```

## Command-Line Interface

### Main Emulator (`bin/mos6502`)

```bash
# Basic usage
bin/mos6502 -f <file> -a <load_addr> [options]

# Options
-f, --file <path>          Binary file to load
-a, --address <hex>        Load address (hex, no 0x prefix)
-c, --cpu <variant>        CPU variant: nmos, 6502, cmos, 65c02 (default: nmos)
-t, --trace                Enable instruction-by-instruction trace
-m, --monitor              Launch interactive TUI monitor
-r, --ram-start <hex>      RAM region start address (default: 0000)
-R, --ram-size <dec>       RAM region size in bytes (default: 32768)
-s, --rom-start <hex>      ROM region start address (default: 8000)
-S, --rom-size <dec>       ROM region size in bytes (default: 32768)

# Examples
bin/mos6502 -f program.bin -a 8000                     # Load at $8000, run
bin/mos6502 -f program.bin -a C000 -t                  # Load at $C000, trace
bin/mos6502 -f program.bin -a 8000 -c 65c02            # Run as CMOS 65C02
bin/mos6502 -f program.bin -a 8000 -m                  # Launch TUI monitor
bin/mos6502 -f program.bin -a 8000 -r 0 -R 16384 -s 4000 -S 16384  # Custom memory map
```

### Memory Layout Defaults

Default configuration (can override with `-r`, `-R`, `-s`, `-S`):
```
$0000-$7FFF: RAM (32KB)
  $0000-$00FF: Zero Page
  $0100-$01FF: Stack
  $0200-$7FFF: General RAM
$8000-$FFFF: ROM (32KB)
  $8000-$FFF9: Program space
  $FFFA-$FFFB: NMI vector
  $FFFC-$FFFD: Reset vector
  $FFFE-$FFFF: IRQ/BRK vector
```

## File Modification Guidelines

### When Modifying Core Files

**`src/cpu.c`** (CPU core):
- Affects: Fetch/decode/execute cycle, variant handling
- Test with: `make run-functional-test`, `make run-test-65c02`
- Watch for: Cycle count changes, interrupt handling, PC increment

**`src/instructions_*.c`** (Instruction system):
- Affects: Opcode behavior, timing, flag updates
- Test with: All test suites (verify, functional, 65c02)
- Watch for: Flag correctness (especially N, Z, V, C), cycle counts

**`src/memory.c`** (Memory system):
- Affects: All memory access, region management
- Test with: All test suites (depends on working memory)
- Watch for: Region overlap, ROM write protection, null pointer returns

**`src/bus.c`** (Bus interface):
- Affects: All CPU operations (reads/writes)
- Test with: All test suites
- Watch for: Breaking abstraction, context handling

**`src/interrupt.c`** (Interrupt controller):
- Affects: IRQ, NMI, BRK behavior
- Test with: `make run-interrupt-test`, functional tests
- Watch for: Edge detection (NMI), masking (IRQ), priority

**`src/addressing.c`** (Addressing modes):
- Affects: All instructions with operands
- Test with: Functional tests (comprehensive addressing mode coverage)
- Watch for: Page-crossing detection, zero-page wrapping, variant differences

**`src/debugger.c`** (Debugging tools):
- Affects: Development experience, profiling
- Test with: `make run-debug-test`
- Watch for: Performance impact (profiling overhead)

**`src/tui_monitor.c`** (TUI monitor):
- Affects: Interactive debugging
- Test with: Manual testing (`make monitor`)
- Watch for: ncurses errors, terminal size requirements (80×19 minimum)

### When Adding New Files

1. **Create header** (`include/newfile.h`):
   ```c
   #ifndef NEWFILE_H
   #define NEWFILE_H

   #include "types.h"

   // Public API declarations
   void newfile_init(void);

   #endif
   ```

2. **Create source** (`src/newfile.c`):
   ```c
   #include "newfile.h"
   #include "cpu.h"
   #include "memory.h"
   // ... other includes

   // Static helpers
   static void helper_function(void) {
       // ...
   }

   // Public API
   void newfile_init(void) {
       // ...
   }
   ```

3. **Update Makefile**:
   - Sources auto-detected via `$(wildcard $(SRC_DIR)/*.c)`
   - Headers auto-detected via `-Iinclude`
   - No changes needed unless adding special build rules

4. **Document in README.md** (if user-facing feature)

## Git Workflow

### Branches

- **Main branch**: Stable, tested code
- **Feature branches**: `claude/<descriptive-name>-<session-id>`
- **Current branch**: `claude/claude-md-mibdnx2476gfifwg-01BCteGTnMh3FVuSNm8jfoqV`

### Commit Guidelines

**Good commit messages**:
```
Add WAI/STP instruction support for 65C02

- Implement handler_WAI and handler_STP
- Add CPU waiting/stopped state tracking
- Update instruction table with new opcodes $CB (WAI) and $DB (STP)
- Test with manual 65C02 test program
```

**Bad commit messages**:
```
Fixed stuff
Update
WIP
```

### Before Committing

1. **Build successfully**:
   ```bash
   make clean
   make
   ```

2. **Run tests**:
   ```bash
   make verify
   make run-functional-test
   make run-test-65c02  # If touching 65C02 code
   ```

3. **Check for warnings**:
   ```bash
   make clean
   make 2>&1 | grep warning
   ```
   Should be zero warnings

4. **Verify no unintended changes**:
   ```bash
   git status
   git diff
   ```

### Push Protocol

```bash
# Standard push
git push -u origin <branch-name>

# Retry on network errors (exponential backoff: 2s, 4s, 8s, 16s)
git push -u origin <branch-name> || sleep 2 && git push -u origin <branch-name> || sleep 4 && git push -u origin <branch-name>
```

**CRITICAL**: Branch must start with `claude/` and end with session ID, or push will fail with 403.

## Performance Considerations

### Emulation Speed

Target: ~1-2 MHz emulated speed on modern hardware (tested on CS student's development machine)

**Optimizations**:
- Instruction dispatch via function pointer table (fast)
- Region-based memory (minimal bounds checking overhead)
- Inline flag macros (no function call overhead)
- Compiled with `-O2` (good balance of speed and debuggability)

**Performance monitoring**:
- TUI monitor shows real-time speed (current, average, 1% low, 0.1% low)
- Profiler shows cycle counts per instruction

**Don't optimize prematurely**:
- Correctness > Speed
- Klaus Dormann tests must pass before optimizing

### Memory Usage

- **Physical memory allocation**: ~64KB for emulated RAM/ROM (configurable)
- **Debugger overhead**: ~10-50KB (breakpoint/watchpoint/profiling data)
- **TUI monitor overhead**: ncurses buffers (~10KB)

**Memory-constrained environments**:
- Reduce RAM/ROM regions
- Disable debugger (`#ifdef ENABLE_DEBUGGER` guards could be added)
- Disable TUI monitor (don't link `tui_monitor.c`)

## External Dependencies

### Required
- **C99 compiler**: Clang (primary) or GCC
- **Make**: GNU Make or compatible
- **ncurses**: TUI monitor support (`libncurses-dev` on Debian/Ubuntu)

### Optional
- **Python 3**: Building example ROMs (`tests/minimal/build_*.py`)
- **curl**: Downloading Klaus Dormann tests (`make download-functional-test`)

### Installing Dependencies

**Debian/Ubuntu**:
```bash
sudo apt-get install clang make libncurses-dev python3 curl
```

**macOS**:
```bash
brew install llvm make ncurses python3 curl
```

**Alpine Linux**:
```bash
apk add clang make ncurses-dev python3 curl
```

## Useful Resources

### 6502 Documentation
- [6502.org](http://6502.org/): Instruction reference, datasheets
- [Visual 6502](http://visual6502.org/): Transistor-level simulation
- [MOS 6502 Programming Manual](http://archive.6502.org/books/mcs6500_family_programming_manual.pdf): Official manual

### 65C02 Documentation
- [WDC 65C02 Datasheet](https://www.westerndesigncenter.com/wdc/documentation/w65c02s.pdf): CMOS variant specs
- [Rockwell 65C02 Extensions](http://6502.org/tutorials/65c02opcodes.html): Bit manipulation instructions

### Testing
- [Klaus Dormann Test Suite](https://github.com/Klaus2m5/6502_65C02_functional_tests): Comprehensive functional tests

### Community
- Author: Korean CS Sophomore (see README footer for context)
- Issues: `https://github.com/MercuriusDream/C99-6502/issues` (inferred, not confirmed)

## Quick Reference: Common Tasks

### I want to...

**...run a program**:
```bash
bin/mos6502 -f program.bin -a 8000
```

**...debug a program**:
```bash
bin/mos6502 -f program.bin -a 8000 -m  # TUI monitor
# OR
bin/mos6502 -f program.bin -a 8000 -t  # Trace
```

**...test CMOS 65C02**:
```bash
bin/mos6502 -f program.bin -a 8000 -c 65c02
```

**...verify emulator correctness**:
```bash
make verify
make run-functional-test
```

**...add a new instruction**:
1. Edit `src/instructions_table.c` (metadata)
2. Edit `src/instructions_implementation.c` (handler)
3. Edit `src/instructions_handlers.c` (dispatch table)
4. Edit `include/instructions_implementation.h` (declaration)
5. Test with trace: `bin/mos6502 -f test.bin -a 8000 -t`

**...profile code execution**:
```c
profiler_record_instruction(REG.PC, OPCODE, cycles);
// ... run program ...
profiler_dump_stats();
profiler_dump_hotspots(20);
```

**...implement a custom device**:
```c
MEM_WORD device_read(MEM_TWO_WORDS addr, void* ctx) {
    // Custom logic
    return 0xFF;
}

void device_write(MEM_TWO_WORDS addr, MEM_WORD data, void* ctx) {
    // Custom logic
}

mem_region_add_io(0x6000, 0x1000, device_read, device_write, context);
```

**...change memory layout**:
```bash
bin/mos6502 -f program.bin -a 4000 -r 0000 -R 16384 -s 4000 -S 16384
# 16KB RAM ($0000-$3FFF), 16KB ROM ($4000-$7FFF)
```

**...understand a function**:
1. Check header file (`include/*.h`) for API documentation
2. Check source file (`src/*.c`) for implementation
3. Check README.md for high-level explanation
4. Check this file (CLAUDE.md) for context

**...fix a bug**:
1. Write a failing test
2. Run with trace: `bin/mos6502 -f test.bin -a 8000 -t`
3. Use TUI monitor: `bin/mos6502 -f test.bin -a 8000 -m`
4. Add debugger breakpoints/watchpoints
5. Fix implementation
6. Verify test passes
7. Run full test suite: `make verify && make run-functional-test`

## Summary: AI Assistant Checklist

When working on C99-6502, ensure you:

- [ ] Follow naming conventions (ALL_CAPS globals, T_ typedefs, lowercase functions)
- [ ] Use 4 spaces for indentation (never tabs)
- [ ] Check CPU variant when implementing/modifying instructions
- [ ] Update all three instruction tables when adding opcodes
- [ ] Test with both NMOS and CMOS modes if touching variant-specific code
- [ ] Run functional tests after major changes
- [ ] Call `mem_region_init()` after configuring memory regions
- [ ] Use type aliases (`MEM_WORD`, `MEM_TWO_WORDS`) instead of raw types
- [ ] Use flag macros (`SET_FLAG`, `CLR_FLAG`, `GET_FLAG`) instead of bitwise ops
- [ ] Preserve zero-page wrapping behavior in addressing modes
- [ ] Don't remove Korean comments in existing code
- [ ] Document new features in README.md if user-facing
- [ ] Build cleanly with zero warnings
- [ ] Commit with descriptive messages
- [ ] Push to branches starting with `claude/` and ending with session ID

---

**Last Updated**: 2025-11-23
**Codebase Version**: Based on commit `c9fd4d2` (65C02 instructions update)
**For Questions**: Refer to README.md or source code comments
