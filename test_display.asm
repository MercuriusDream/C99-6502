; Simple test to write "HELLO WORLD" to Apple II screen
; Screen memory starts at $0400
; Characters are in Apple II format (high bit set)

* = $0800

START:
    LDX #$00           ; Index into message

LOOP:
    LDA MESSAGE,X      ; Load character from message
    BEQ DONE           ; If zero, we're done
    STA $0400,X        ; Store to screen memory
    INX                ; Next character
    JMP LOOP           ; Continue loop

DONE:
    JMP DONE           ; Infinite loop

MESSAGE:
    .BYTE $C8, $C5, $CC, $CC, $CF, $A0  ; "HELLO "
    .BYTE $D7, $CF, $D2, $CC, $C4, $00  ; "WORLD" + null terminator
