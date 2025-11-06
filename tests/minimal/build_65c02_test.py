rom = bytearray()

# Test 1: BRA - Branch Always
rom.extend([
    0xA9, 0x01,        # LDA #$01
    0x80, 0x02,        # BRA +2 (skip next instruction)
    0xA9, 0xFF,        # LDA #$FF (should be skipped)
    0x8D, 0x00, 0x02,  # STA $0200
])

# Test 2: PHX/PLX - Push/Pull X
rom.extend([
    0xA2, 0x42,        # LDX #$42
    0xDA,              # PHX
    0xA2, 0x00,        # LDX #$00
    0xFA,              # PLX
    0x8E, 0x01, 0x02,  # STX $0201
])

# Test 3: PHY/PLY - Push/Pull Y
rom.extend([
    0xA0, 0x43,        # LDY #$43
    0x5A,              # PHY
    0xA0, 0x00,        # LDY #$00
    0x7A,              # PLY
    0x8C, 0x02, 0x02,  # STY $0202
])

# Test 4: STZ Zero Page
rom.extend([
    0xA9, 0xFF,        # LDA #$FF
    0x85, 0x10,        # STA $10
    0x64, 0x10,        # STZ $10
    0xA5, 0x10,        # LDA $10
    0x8D, 0x03, 0x02,  # STA $0203
])

# Test 5: STZ Absolute
rom.extend([
    0xA9, 0xFF,        # LDA #$FF
    0x8D, 0x04, 0x02,  # STA $0204
    0x9C, 0x04, 0x02,  # STZ $0204
])

# Test 6: TSB - Test and Set Bits
rom.extend([
    0xA9, 0x0F,        # LDA #$0F (0000_1111)
    0x85, 0x20,        # STA $20
    0xA9, 0xF0,        # LDA #$F0 (1111_0000)
    0x04, 0x20,        # TSB $20    ; $20 = $0F | $F0 = $FF
    0xA5, 0x20,        # LDA $20
    0x8D, 0x05, 0x02,  # STA $0205
])

# Test 7: TRB - Test and Reset Bits
rom.extend([
    0xA9, 0xFF,        # LDA #$FF
    0x85, 0x21,        # STA $21
    0xA9, 0xF0,        # LDA #$F0
    0x14, 0x21,        # TRB $21    ; $21 = $FF & ~$F0 = $0F
    0xA5, 0x21,        # LDA $21
    0x8D, 0x06, 0x02,  # STA $0206
])

# Test 8: STZ Zero Page,X
rom.extend([
    0xA2, 0x05,        # LDX #$05
    0xA9, 0xFF,        # LDA #$FF
    0x95, 0x30,        # STA $30,X  ; Store at $35
    0x74, 0x30,        # STZ $30,X  ; Clear $35
    0xB5, 0x30,        # LDA $30,X  ; Load from $35
    0x8D, 0x07, 0x02,  # STA $0207
])

# Test 9: STZ Absolute,X
rom.extend([
    0xA2, 0x03,        # LDX #$03
    0xA9, 0xFF,        # LDA #$FF
    0x9D, 0x08, 0x02,  # STA $0208,X ; Store at $020B
    0x9E, 0x08, 0x02,  # STZ $0208,X ; Clear $020B
])

# End marker
rom.extend([
    0x00,              # BRK
])

# Pad to 256 bytes
rom.extend([0xEA] * (256 - len(rom)))

# Write ROM
with open('65c02_test.bin', 'wb') as f:
    f.write(rom)

print(f"""65C02 test ROM built, {len(rom)} bytes
Expected results:
$0200: $01  (BRA test - skipped $FF)
$0201: $42  (PHX/PLX)
$0202: $43  (PHY/PLY)
$0203: $00  (STZ ZP)
$0204: $00  (STZ ABS)
$0205: $FF  (TSB - set bits)
$0206: $0F  (TRB - reset bits)
$0207: $00  (STZ ZP,X)
$020B: $00  (STZ ABS,X)
Run with: ./bin/mos6502 -f tests/minimal/65c02_test.bin -a 8000 -c 65c02""")