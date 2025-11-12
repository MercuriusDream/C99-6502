; Simple interactive test program for Apple II
; This will display a prompt and echo keypresses

        .org $0800

start:
        ; Clear screen and show prompt
        lda #$8D        ; Carriage return
        jsr cout
        lda #$8D
        jsr cout

        ; Print "READY" message
        ldx #0
msg_loop:
        lda message,x
        beq wait_key
        jsr cout
        inx
        bne msg_loop

wait_key:
        ; Wait for keypress
        lda $C000       ; Read keyboard
        bpl wait_key    ; Wait until key pressed

        sta $C010       ; Clear keyboard strobe

        ; Echo the key
        and #$7F        ; Clear high bit
        ora #$80        ; Set high bit for display
        jsr cout

        jmp wait_key    ; Loop forever

; Character output routine
cout:
        sta $0400       ; Just write to top-left for now
        rts

message:
        .byte $D2,$C5,$C1,$C4,$D9,$A0,$BE,$A0,$00  ; "READY >"

        .end
