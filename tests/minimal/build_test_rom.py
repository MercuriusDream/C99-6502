def build_rom():
    rom = bytearray()

    # offsets
    start_addr = 0x8000

    # START: $8000
    rom.extend([
        0xA9, 0x42,        # LDA #$42
        0x8D, 0x00, 0x02,  # STA $0200
        0xA2, 0x10,        # LDX #$10
        0x8E, 0x01, 0x02,  # STX $0201
        0xA0, 0x20,        # LDY #$20
        0x8C, 0x02, 0x02,  # STY $0202
    ])

    # register tests 
    rom.extend([
        0xAA,              # TAX
        0xA8,              # TAY
        0x8A,              # TXA
    ])

    # arithmetic tests
    rom.extend([
        0xA9, 0x10,        # LDA #$10
        0x18,              # CLC
        0x69, 0x05,        # ADC #$05
        0x8D, 0x03, 0x02,  # STA $0203
        0x38,              # SEC
        0xE9, 0x03,        # SBC #$03
        0x8D, 0x04, 0x02,  # STA $0204
    ])

    # logical operation tests
    rom.extend([
        0xA9, 0x0F,        # LDA #$0F
        0x29, 0x03,        # AND #$03
        0x8D, 0x05, 0x02,  # STA $0205
        0xA9, 0x05,        # LDA #$05
        0x09, 0x02,        # ORA #$02
        0x8D, 0x06, 0x02,  # STA $0206
        0xA9, 0xFF,        # LDA #$FF
        0x49, 0x0F,        # EOR #$0F
        0x8D, 0x07, 0x02,  # STA $0207
    ])

    # increment and ecrement operation tests
    rom.extend([
        0xA2, 0x00,        # LDX #$00
        0xE8,              # INX
        0xE8,              # INX
        0xE8,              # INX
        0x8E, 0x08, 0x02,  # STX $0208
        0xA0, 0x05,        # LDY #$05
        0x88,              # DEY
        0x88,              # DEY
        0x8C, 0x09, 0x02,  # STY $0209
    ])

    # branching tests
    rom.extend([
        0xA9, 0x00,        # LDA #$00
        0xC9, 0x00,        # CMP #$00
        0xF0, 0x02,        # BEQ += 2
        0xA9, 0xFF,        # LDA #$FF, Should be skipped
        
        0xA9, 0xAA,        # LDA #$AA
        0x8D, 0x0A, 0x02,  # STA $020A
    ])

    # subroutine call tests
    # current position = 0x57, will be increased to 0x5A after JSR
    # place multiply right after the main code
    # add JSR's placeholder first, then fill the address later
    jsr_pos = len(rom)
    rom.extend([
        0x20, 0x00, 0x00,  # JSR, address to be filled
        0x8D, 0x0B, 0x02,  # STA $020B
    ])

    # stack tests
    rom.extend([
        0xA9, 0x55,        # LDA #$55
        0x48,              # PHA
        0xA9, 0x66,        # LDA #$66
        0x68,              # PLA
        0x8D, 0x0C, 0x02,  # STA $020C
    ])

    # indexed addressing
    # will place DATA at the end to calculate the address
    data_jsr_pos = len(rom)
    rom.extend([
        0xA2, 0x03,        # LDX #$03
        0xBD, 0x00, 0x00,  # LDA (tbd),X
        0x8D, 0x0D, 0x02,  # STA $020D
    ])

    # zpi test
    rom.extend([
        0xA9, 0x99,        # LDA #$99
        0x85, 0x10,        # STA $10
        0xA2, 0x00,        # LDX #$00
        0xB5, 0x10,        # LDA $10,X
        0x8D, 0x0E, 0x02,  # STA $020E
    ])

    # end test
    rom.extend([
        0x00,              # BRK
    ])

    # multiply subroutine
    multiply_addr = start_addr + len(rom)
    rom.extend([
        0xA9, 0x00,        # LDA #$00
        0x18,              # CLC
        0x69, 0x02,        # ADC #$02
        0x69, 0x02,        # ADC #$02
        0x69, 0x02,        # ADC #$02
        0x60,              # RTS
    ])

    # DATA
    data_addr = start_addr + len(rom)
    rom.extend([
        0x11, 0x22, 0x33, 0x44, 0x55,  # data bytes
    ])

    # fill in the JSR address for multiply
    rom[jsr_pos + 1] = multiply_addr & 0xFF
    rom[jsr_pos + 2] = (multiply_addr >> 8) & 0xFF

    # Filling the LDA address for DATA
    # data_jsr_pos is pointing to LDX #$03 (2 bytes)
    # LDA addr,X after that (3 bytes: opcode + address)
    rom[data_jsr_pos + 3] = data_addr & 0xFF
    rom[data_jsr_pos + 4] = (data_addr >> 8) & 0xFF

    # actually write the binary to the file
    with open('test.bin', 'wb') as f:
        f.write(rom)

    # Printing the results
    print(f"Test ROM built, {len(rom)} bytes")

    print(f"""Memory layout
    START:     $8000
    MULTIPLY:  ${multiply_addr:04X}
    DATA:      ${data_addr:04X}
    END:       ${start_addr + len(rom):04X}""")

    print(f"""\Expected results in memory:
    $0200: $42  (LDA #$42, STA $0200)
    $0201: $10  (LDX #$10, STX $0201)
    $0202: $20  (LDY #$20, STY $0202)
    $0203: $15  (ADC test: $10 + $05)
    $0204: $12  (SBC test: $15 - $03)
    $0205: $03  (AND test: $0F & $03)
    $0206: $07  (ORA test: $05 | $02)
    $0207: $F0  (EOR test: $FF ^ $0F)
    $0208: $03  (INX x3)")
    $0209: $03  (DEY x2 from $05)")
    $020A: $AA  (Branch test)")
    $020B: $06  (Subroutine: 2+2+2)")
    $020C: $55  (Stack test)")
    $020D: $44  (Indexed: DATA[3])
    $020E: $99  (Zero page indexed)""")

    print(f"\nRun with ./mos6502 -f test.bin -a 8000 -t")

if __name__ == '__main__':
    build_rom()