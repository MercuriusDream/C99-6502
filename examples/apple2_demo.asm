; Apple II Demo Program
; Demonstrates Apple II peripherals

.ORG $C000

START:
    ; Write "HELLO" to screen (Apple II text screen)
    ; Apple II uses high-bit ASCII ($C0-$DF for uppercase)
    LDA #$C8          ; 'H'
    STA $0400
    LDA #$C5          ; 'E'
    STA $0401
    LDA #$CC          ; 'L'
    STA $0402
    STA $0403
    LDA #$CF          ; 'O'
    STA $0404

    ; Read keyboard (simulated)
    LDA $C000         ; Apple II keyboard read address
    STA $0405         ; Display keyboard value

    ; Do some calculations
    LDX #$05
CALC_LOOP:
    TXA
    ASL               ; Multiply by 2
    STA $0500,X       ; Store at screen location
    DEX
    BPL CALC_LOOP

    ; Final message
    LDA #$A0          ; Space
    STA $0440
    LDA #$C4          ; 'D'
    STA $0441
    LDA #$CF          ; 'O'
    STA $0442
    LDA #$CE          ; 'N'
    STA $0443
    LDA #$C5          ; 'E'
    STA $0444

    BRK
