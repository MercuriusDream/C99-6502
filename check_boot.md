# Boot Process Analysis

## What should happen:
1. CPU starts at $C600 (Disk II ROM)
2. Disk II ROM reads Track 0, Sector 0 (boot sector)
3. Boot sector is loaded to $0800-$08FF  
4. Disk II ROM jumps to $0801 to execute boot code
5. Boot code loads ProDOS and runs it

## What's happening:
- PC stuck at $FCAA (Monitor ROM delay loop) and $C662-$C666 (Disk II read)
- Disk sectors ARE being read (we see the [DISK] messages)
- But the boot never progresses past the initial read

## Problem:
The Disk II ROM is reading data but something about the encoding or the data itself
is preventing the boot sector from being recognized as valid.

Possible issues:
1. Nibble encoding is wrong
2. Boot sector isn't being written to $0800
3. Checksum validation is failing
4. Sector timing/sequencing is off

