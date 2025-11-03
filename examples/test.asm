; MOS 6502 Test ROM
; Load address: $8000
; Tests various 6502 instructions and addressing modes

.org $8000

START:
    ; Test 1: Load and Store operations
    LDA #$42        ; Load immediate
    STA $0200       ; Store to memory
    LDX #$10        ; Load X
    STX $0201       ; Store X
    LDY #$20        ; Load Y
    STY $0202       ; Store Y

    ; Test 2: Register transfers
    TAX             ; Transfer A to X
    TAY             ; Transfer A to Y
    TXA             ; Transfer X to A

    ; Test 3: Arithmetic operations
    LDA #$10        ; Load $10
    CLC             ; Clear carry
    ADC #$05        ; Add $05 (result: $15)
    STA $0203       ; Store result

    SEC             ; Set carry
    SBC #$03        ; Subtract $03 (result: $12)
    STA $0204       ; Store result

    ; Test 4: Logical operations
    LDA #$0F        ; Load $0F
    AND #$03        ; AND with $03 (result: $03)
    STA $0205       ; Store result

    LDA #$05        ; Load $05
    ORA #$02        ; OR with $02 (result: $07)
    STA $0206       ; Store result

    LDA #$FF        ; Load $FF
    EOR #$0F        ; XOR with $0F (result: $F0)
    STA $0207       ; Store result

    ; Test 5: Increment/Decrement
    LDX #$00
    INX             ; X = $01
    INX             ; X = $02
    INX             ; X = $03
    STX $0208       ; Store X

    LDY #$05
    DEY             ; Y = $04
    DEY             ; Y = $03
    STY $0209       ; Store Y

    ; Test 6: Branching
    LDA #$00
    CMP #$00        ; Compare with zero
    BEQ EQUAL       ; Branch if equal (should take)
    LDA #$FF        ; Should NOT execute
EQUAL:
    LDA #$AA        ; Should execute
    STA $020A       ; Store result

    ; Test 7: Subroutine call
    JSR MULTIPLY    ; Call subroutine
    STA $020B       ; Store result

    ; Test 8: Stack operations
    LDA #$55
    PHA             ; Push A to stack
    LDA #$66
    PLA             ; Pull from stack (A = $55)
    STA $020C       ; Store result

    ; Test 9: Indexed addressing
    LDX #$03
    LDA DATA,X      ; Load from DATA + X
    STA $020D       ; Store result

    ; Test 10: Zero page indexed
    LDA #$99
    STA $10
    LDX #$00
    LDA $10,X       ; Zero page indexed load
    STA $020E       ; Store result

    ; End test
    BRK             ; Break

; Subroutine: Multiply 2 * 3 = 6
MULTIPLY:
    LDA #$00        ; Initialize accumulator
    CLC
    ADC #$02        ; Add 2
    ADC #$02        ; Add 2
    ADC #$02        ; Add 2 (total = 6)
    RTS             ; Return

; Data section
DATA:
    .byte $11, $22, $33, $44, $55

; Reset vector (points to START)
.org $FFFC
.word START

; IRQ vector
.org $FFFE
.word START
