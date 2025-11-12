# Additional Tools

## Interactive Debugger

GDB-style interactive debugger for cycle-accurate debugging.

### Build

```bash
make debugger
```

### Usage

```bash
bin/mos6502-debug -f program.bin -a 8000 [-c 65c02]
```

### Commands

| Command | Description |
|---------|-------------|
| `s [n]` | Step n instructions (default: 1) |
| `c` | Continue until breakpoint |
| `b <addr>` | Set breakpoint at address |
| `w <addr>` | Watch memory address |
| `r` | Show registers |
| `d [addr] [n]` | Disassemble n instructions |
| `x [addr] [n]` | Examine n bytes |
| `stack` | Show stack |
| `lb`, `lw` | List breakpoints/watchpoints |
| `cb`, `cw` | Clear breakpoints/watchpoints |
| `reset` | Reset CPU |
| `q` | Quit |

## Assembler

Two-pass assembler with label resolution.

### Build

```bash
make assembler
```

### Usage

```bash
bin/mos6502-asm input.asm -o output.bin [-l] [-c 65c02]
```

Options:
- `-l` - Print assembly listing
- `-c 65c02` - Use 65C02 instruction set

### Syntax

```asm
.ORG $8000        ; Set origin

LABEL:            ; Define label
    LDX #$00      ; Immediate: #$value
    LDA $00       ; Zero page: $addr
    STA $0200,X   ; Indexed: $addr,X
    JMP (VECTOR)  ; Indirect: ($addr)
    BEQ LABEL     ; Relative branch
```

Supported addressing modes: Implied, Accumulator, Immediate (#), Zero Page, Zero Page Indexed (,X/,Y), Absolute, Absolute Indexed (,X/,Y), Indirect (JMP), Indexed Indirect ((,X)), Indirect Indexed ((,),Y), Relative (branches).

All standard 6502 opcodes plus 65C02 extensions (BRA, PHX, PHY, PLX, PLY, STZ, TRB, TSB, WAI, STP) are supported.

## System Configurations

Pre-configured memory maps and peripheral handlers for classic 6502 systems.

### Available Configurations

| System | CPU | RAM | ROM | Peripherals |
|--------|-----|-----|-----|-------------|
| Apple II | 6502 | 48KB | 16KB | Keyboard ($C000) |
| NES | 6502 | 2KB | 32KB | PPU ($2000-$2007), APU ($4000-$4017) |
| C64 | 6502 | 48KB | 8KB | VIC-II ($D000), SID ($D400) |

### Usage

```bash
# Run emulator in Apple II mode
bin/mos6502 --system apple2 -f program.bin -a C000

# Debug in NES mode
bin/mos6502-debug --system nes -f game.bin -a 8000

# Run in C64 mode
bin/mos6502 --system c64 -f program.bin -a A000
```

System names: `apple2`, `nes`, `c64`

### Location

System-specific code in `systems/` directory:
- `systems/apple2/apple2_config.c`
- `systems/nes/nes_config.c`
- `systems/c64/c64_config.c`

I/O writes to peripheral addresses are logged to stdout for debugging.

## Development Workflow

```bash
# Write assembly
cat > program.asm <<'EOF'
.ORG $8000
    LDX #$00
LOOP:
    INX
    CPX #$10
    BNE LOOP
    BRK
EOF

# Assemble
bin/mos6502-asm program.asm -o program.bin -l

# Debug
bin/mos6502-debug -f program.bin -a 8000
```

Debugger commands:
```
(6502) b 8004        # Set breakpoint
(6502) c             # Continue to breakpoint
(6502) s 5           # Step 5 instructions
(6502) x 0200 16     # Examine memory
(6502) r             # Show registers
```

## Disk Image Tool

Read and extract data from Apple II .2mg disk images.

### Build

```bash
make disk-info
```

### Usage

```bash
# Show disk information
bin/disk-info disk.2mg -i

# Dump specific block
bin/disk-info disk.2mg -b 0

# Extract data at offset
bin/disk-info disk.2mg -e 0800 -s 4096
```

Supports DOS 3.3 and ProDOS format .2mg files. See [DISK_IMAGES.md](DISK_IMAGES.md) for details.

### Workflow

```bash
# 1. Inspect disk
bin/disk-info myprogram.2mg -i

# 2. Extract program (if you know offset)
bin/disk-info myprogram.2mg -e 0800 -s 2048

# 3. Run extracted program
bin/mos6502 --system apple2 -f extracted_0800.bin -a 0800
```

## Files

### Headers
- `include/interactive_debugger.h`
- `include/assembler.h`
- `include/system_config.h`
- `include/disk_image.h`

### Implementation
- `src/interactive_debugger.c`
- `src/assembler.c`
- `src/system_config.c`
- `src/disk_image.c`
- `systems/apple2/apple2_config.c`
- `systems/nes/nes_config.c`
- `systems/c64/c64_config.c`
- `tools/disk_info.c`

### Binaries
- `bin/mos6502-debug` - Interactive debugger
- `bin/mos6502-asm` - Assembler
- `bin/disk-info` - Disk image tool

### Examples
- `examples/hello.asm` - Example program
- `examples/apple2_demo.asm` - Apple II demo
