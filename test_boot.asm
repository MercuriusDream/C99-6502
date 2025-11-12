.ORG $C600

; Simple disk bootloader test
; This will read track 0, sectors 0-1 (512 bytes) into $0800 and jump to it

START:
    ; Turn on motor
    LDA $C0E9

    ; Set phase 0 (track 0)
    LDA $C0E1

    ; Wait a bit for motor to spin up
    LDX #$10
DELAY:
    LDY #$FF
DELAY2:
    DEY
    BNE DELAY2
    DEX
    BNE DELAY

    ; Read 512 bytes from disk (two sectors)
    ; First 256 bytes to $0800
    LDX #$00
READ_LOOP1:
    LDA $C0EC       ; Set Q6L, Q7L for read mode, read data
    STA $0800,X     ; Store at $0800
    INX
    BNE READ_LOOP1

    ; Second 256 bytes to $0900
    LDX #$00
READ_LOOP2:
    LDA $C0EC       ; Read next sector data
    STA $0900,X     ; Store at $0900
    INX
    BNE READ_LOOP2

    ; Jump to boot code
    JMP $0801
