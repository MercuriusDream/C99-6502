; Simple screen test for Apple II
; Writes "HELLO APPLE II" to screen and loops

*=$8000

start:
    LDX #$00
loop:
    LDA message,X
    BEQ done
    STA $0400,X    ; Write to screen memory
    INX
    JMP loop

done:
    JMP done       ; Loop forever

message:
    .byte $C8,$C5,$CC,$CC,$CF,$A0,$C1,$D0,$D0,$CC,$C5,$A0,$C9,$C9,$00
    ; "HELLO APPLE II" in Apple II encoding ($C0-$DF = normal uppercase)
