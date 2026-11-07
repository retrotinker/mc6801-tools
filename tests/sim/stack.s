; stack.s - subroutines, the stack, SWI and RTI
;
; Results are stored in RAM from $80 on; the "expect" comments give the
; values worked out from the MC6801 data sheet, which the reference
; output checks.  A return address is checked against its label, giving
; $01 if it is right, so the results don't depend on where the code is.

        ORG     $1000
start   lds     #$00FF
        ldx     #handler        ; the SWI vector
        stx     $FFFA

; PSHX and PULX                                     expect 12 34
        ldx     #$1234
        pshx
        ldx     #0
        pulx
        stx     $80
; PSHA and PULB                                     expect AB
        ldaa    #$AB
        psha
        pulb
        stab    $82
; BSR pushes the address of the next instruction    expect 01
        bsr     sub1
ret1    nop
; JSR: SP inside the subroutine, return address     expect 00 FD 01
        jsr     sub2
ret2    nop
; JSR indexed                                       expect 5A
        ldx     #sub3
        jsr     0,x
; SWI and RTI: the handler adds 1 to the stacked A, and RTI restores
; A, B, X, SP and CC from the stack; in the handler I is set
;                         expect 12 22 33 44 00 FF D0 C0 C0
        ldaa    #$C0
        tap
        ldaa    #$11
        ldab    #$22
        ldx     #$3344
        swi
        psha
        tpa
        staa    $90             ; CC after RTI
        pula
        staa    $88
        stab    $89
        stx     $8A
        sts     $8C
; JMP indexed                                       expect 01
        ldx     #jmp1
        jmp     0,x
        clr     $91
        bra     done1
jmp1    ldaa    #1
        staa    $91
done1
; DES and INS                                       expect 00 FD 00 FF
        des
        des
        sts     $92
        ins
        ins
        sts     $94
        bra     *

sub1    tsx                     ; X points at the return address
        ldx     0,x
        clr     $83
        cpx     #ret1
        bne     sub1x
        inc     $83
sub1x   rts

sub2    sts     $84
        tsx
        ldx     0,x
        clr     $86
        cpx     #ret2
        bne     sub2x
        inc     $86
sub2x   rts

sub3    ldaa    #$5A
        staa    $87
        rts

; the stack holds CC, B, A, X (high, low) and PC (high, low)
handler tpa
        staa    $8E             ; CC in the handler
        tsx
        ldaa    0,x
        staa    $8F             ; CC as stacked
        inc     2,x             ; the stacked A
        ldaa    #$99            ; RTI restores A
        rti
