; Apple II Test Program
; Demonstrates keyboard input and text screen output

.ORG $C000

START:
    LDX #$00          ; Initialize X counter
    LDY #$00          ; Initialize Y position

WRITE_LOOP:
    LDA MESSAGE,X     ; Load character from message
    BEQ DONE          ; If zero, we're done

    STA $0400,Y       ; Write to text screen
    INX               ; Next character
    INY               ; Next screen position
    BNE WRITE_LOOP    ; Continue if Y hasn't wrapped

DONE:
    LDA $C000         ; Read keyboard (Apple II keyboard port)
    STA $0420         ; Store keyboard value on screen

    ; Test some arithmetic
    LDA #$10
    ADC #$20
    STA $0430         ; Store result

    BRK               ; End program

MESSAGE:
    ; "HELLO APPLE II" in ASCII
    ; Note: Apple II uses high-bit ASCII for normal text
    .BYTE $C8, $C5, $CC, $CC, $CF, $A0  ; "HELLO "
    .BYTE $C1, $D0, $D0, $CC, $C5, $A0  ; "APPLE "
    .BYTE $C9, $C9, $00                 ; "II" + terminator
