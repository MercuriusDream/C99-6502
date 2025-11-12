; Simple Apple II Test Program
; Tests keyboard read and screen write

.ORG $C000

START:
    ; Write some values to text screen area
    LDA #$C8          ; 'H' in Apple II high-bit ASCII
    STA $0400         ; Write to screen position 0

    LDA #$C5          ; 'E'
    STA $0401

    LDA #$CC          ; 'L'
    STA $0402

    LDA #$CC          ; 'L'
    STA $0403

    LDA #$CF          ; 'O'
    STA $0404

    ; Read keyboard
    LDA $C000         ; Read Apple II keyboard port
    STA $0410         ; Store on screen

    ; Do some math
    LDA #$42
    ADC #$10
    STA $0420         ; Store result

    ; Test indexed write
    LDX #$00
LOOP:
    TXA
    STA $0500,X       ; Write X value to screen
    INX
    CPX #$10
    BNE LOOP

    BRK
