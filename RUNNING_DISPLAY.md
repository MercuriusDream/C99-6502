# Running the Apple II Emulator with Display

## Current Status

The emulator is configured to boot into the Apple II Monitor ROM with SDL display enabled.

## How to Run

```bash
cd /Users/seong-useog/T_T_MOS_6502_ASM_IMPL
./bin/mos6502 --system apple2
```

## What Should Happen

1. **Console output** will show:
   ```
   C99-6502 [Apple II Mode]
   CPU Variant: NMOS 6502
   Initializing Apple II peripherals...
   SDL display initialized (560x384)
   ...
   [DISPLAY] Test messages written to screen buffer
   ```

2. **SDL Window** should pop up showing:
   - Green text on black background (authentic Apple II look)
   - "APPLE II READY" at the top
   - "MONITOR ACTIVE - TYPE COMMANDS" below
   - The Monitor will be running

3. **Controls:**
   - Type to interact with the Monitor
   - Press ESC to quit

## Troubleshooting

### Window doesn't appear

1. **Check macOS permissions:**
   - System Settings → Privacy & Security → Screen Recording
   - Make sure Terminal has permissions

2. **Check SDL installation:**
   ```bash
   brew list sdl2
   ```

3. **Try running in foreground** (not in background):
   - Make sure you're running the command directly in Terminal
   - Not through SSH or remote session

### Window appears but is blank

- The screen buffer should have test messages written
- If blank, there may be an issue with the display rendering code

### Window appears but Monitor doesn't work

- This is expected - the disk boot was failing
- We changed to boot directly to Monitor at $FA62
- You should see text appear when Monitor initializes

## Disk Booting

### Fixed!

The disk controller now properly:
1. Nibblizes entire tracks (all 16 sectors)
2. Simulates continuous disk rotation by wrapping the read position
3. Allows the Disk II ROM to search for address/data prologues
4. Should successfully boot ProDOS!

### Previous Issue (RESOLVED)

The original implementation only nibblized one sector at a time and auto-advanced
to the next sector after reading. The Disk II ROM expects to:
- Continuously read a rotating track
- Search for sector address fields
- Control which sector to read

The fix was to nibblize the entire track and let the read position wrap around,
simulating a real rotating disk.

## Next Steps

To make disk booting work, you need to fix the disk controller implementation
in `src/disk_controller.c`.

Reference implementations:
- AppleWin: https://github.com/AppleWin/AppleWin
- MAME: Apple II driver
