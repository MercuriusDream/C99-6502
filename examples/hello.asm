; Simple 6502 program demonstrating the assembler
; Counts from 0 to 10 and stores results in memory

.ORG $8000

START:
    LDX #$00      ; Initialize X to 0
    LDY #$00      ; Initialize Y to 0

LOOP:
    TXA           ; Transfer X to A
    STA $0200,Y   ; Store A at $0200 + Y
    INX           ; Increment X
    INY           ; Increment Y
    CPX #$0A      ; Compare X with 10
    BNE LOOP      ; Branch if not equal

DONE:
    BRK           ; Break
