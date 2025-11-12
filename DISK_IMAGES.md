# Using Apple II Disk Images (.2mg)

The emulator supports reading Apple II disk images in .2mg format.

## Disk Info Tool

```bash
make disk-info
```

### Usage

```bash
# Show disk information
bin/disk-info disk.2mg -i

# Dump specific block
bin/disk-info disk.2mg -b 0

# Extract data at specific offset
bin/disk-info disk.2mg -e 0800 -s 2048
```

## .2mg Format

.2mg files contain:
- Header (64 bytes minimum)
- Disk data (DOS 3.3, ProDOS, or NIB format)
- Optional comments and creator data

### Format Types
- **0**: DOS 3.3 sector order
- **1**: ProDOS block order
- **2**: NIB format

## Example Workflow

### 1. Inspect Disk Image

```bash
bin/disk-info myprogram.2mg -i
```

This shows:
- Disk format (DOS 3.3 or ProDOS)
- Size in KB
- First 256 bytes of data

### 2. Extract Program

If you know where a program is located on the disk:

```bash
# Extract from offset $0800, 4KB
bin/disk-info myprogram.2mg -e 0800 -s 4096
```

This creates `extracted_0800.bin`

### 3. Run Extracted Program

```bash
bin/mos6502 --system apple2 -f extracted_0800.bin -a 0800
```

Or debug it:

```bash
bin/mos6502-debug --system apple2 -f extracted_0800.bin -a 0800
```

## Finding Programs on Disk

### DOS 3.3 Disks
- Boot sectors: Track 0, Sectors 0-15
- VTOC (Volume Table of Contents): Track 17, Sector 0
- Catalog: Track 17, Sectors 1-15
- Files are scattered across tracks/sectors

### ProDOS Disks
- Blocks 0-1: Boot blocks
- Block 2: Volume directory
- Files use block allocation

## Creating Test Disk

To create a simple test disk with your program:

```bash
# Assemble your program
bin/mos6502-asm myprogram.asm -o myprogram.bin

# Use a disk image tool (outside this emulator) to:
# 1. Create blank .2mg
# 2. Format as DOS 3.3 or ProDOS
# 3. Copy myprogram.bin to the disk
# 4. Use disk-info to extract and test
```

## Notes

- The tool reads .2mg headers and raw disk data
- Full filesystem support (reading directories, etc.) is not yet implemented
- For now, you need to know file locations on disk
- Use Apple II disk utilities to browse disk contents
- Common Apple II tools: CiderPress, AppleCommander

## Future Enhancements

Potential additions:
- DOS 3.3 filesystem reader
- ProDOS filesystem reader
- File extraction by name
- Disk drive emulation at $C0Ex
