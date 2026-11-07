; arith.s - arithmetic and logical instructions and their condition codes
;
; Each case sets the condition codes to a known value (C0, or C1 with
; carry set), does one operation, and stores the result and then the
; condition codes in RAM from $80 on, two bytes per case (three or four
; for 16-bit results).  The "expect" comments give the values worked
; out from the MC6801 data sheet; the reference output checks them.
; CC bits: 20 H, 10 I, 08 N, 04 Z, 02 V, 01 C (C0 always set).

RES     equ     $80

        ORG     $1000
start   lds     #$00FF
        ldx     #RES

; ADDA $7F+$01: half carry, overflow           expect 80 EA
        ldaa    #$C0
        tap
        ldaa    #$7F
        adda    #$01
        jsr     save
; ADDA $FF+$01: zero, carry, half carry        expect 00 E5
        ldaa    #$C0
        tap
        ldaa    #$FF
        adda    #$01
        jsr     save
; SUBA $80-$01: overflow                       expect 7F C2
        ldaa    #$C0
        tap
        ldaa    #$80
        suba    #$01
        jsr     save
; SUBA $00-$01: negative, borrow               expect FF C9
        ldaa    #$C0
        tap
        ldaa    #$00
        suba    #$01
        jsr     save
; ADCA $0F+$00+C: half carry from the carry    expect 10 E0
        ldaa    #$C1
        tap
        ldaa    #$0F
        adca    #$00
        jsr     save
; SBCA $10-$0F-C: zero                         expect 00 C4
        ldaa    #$C1
        tap
        ldaa    #$10
        sbca    #$0F
        jsr     save
; CMPA $05 with $06: A unchanged               expect 05 C9
        ldaa    #$C0
        tap
        ldaa    #$05
        cmpa    #$06
        jsr     save
; NEGA $80: overflow, carry                    expect 80 CB
        ldaa    #$C0
        tap
        ldaa    #$80
        nega
        jsr     save
; NEGA $00: zero, no carry                     expect 00 C4
        ldaa    #$C1
        tap
        ldaa    #$00
        nega
        jsr     save
; COMA $55: carry always set                   expect AA C9
        ldaa    #$C0
        tap
        ldaa    #$55
        coma
        jsr     save
; INCA $7F: overflow, C unchanged              expect 80 CA
        ldaa    #$C0
        tap
        ldaa    #$7F
        inca
        jsr     save
; DECA $80: overflow, C unchanged              expect 7F C3
        ldaa    #$C1
        tap
        ldaa    #$80
        deca
        jsr     save
; DAA after $19+$28 (BCD 47)                   expect 47 E0
        ldaa    #$C0
        tap
        ldaa    #$19
        adda    #$28
        daa
        jsr     save
; DAA after $99+$01 (BCD 100)                  expect 00 C5
        ldaa    #$C0
        tap
        ldaa    #$99
        adda    #$01
        daa
        jsr     save
; ASRA $81: sign kept                          expect C0 C9
        ldaa    #$C0
        tap
        ldaa    #$81
        asra
        jsr     save
; RORA $02 with carry in                       expect 81 CA
        ldaa    #$C1
        tap
        ldaa    #$02
        rora
        jsr     save
; ROLA $80 with no carry in                    expect 00 C7
        ldaa    #$C0
        tap
        ldaa    #$80
        rola
        jsr     save
; LSRA $01                                     expect 00 C7
        ldaa    #$C0
        tap
        ldaa    #$01
        lsra
        jsr     save
; ASLA $C0                                     expect 80 C9
        ldaa    #$C0
        tap
        ldaa    #$C0
        asla
        jsr     save
; TSTA $80: V and C cleared                    expect 80 C8
        ldaa    #$C3
        tap
        ldaa    #$80
        tsta
        jsr     save
; SBA $10-$20                                  expect F0 C9
        ldaa    #$C0
        tap
        ldaa    #$10
        ldab    #$20
        sba
        jsr     save
; ABA $50+$50: overflow                        expect A0 CA
        ldaa    #$C0
        tap
        ldaa    #$50
        ldab    #$50
        aba
        jsr     save
; ANDA $F0 with $0F: zero, V cleared           expect 00 C5
        ldaa    #$C3
        tap
        ldaa    #$F0
        anda    #$0F
        jsr     save
; EORA $FF with $0F                            expect F0 C8
        ldaa    #$C0
        tap
        ldaa    #$FF
        eora    #$0F
        jsr     save
; ORAA $80 with $01                            expect 81 C8
        ldaa    #$C0
        tap
        ldaa    #$80
        oraa    #$01
        jsr     save
; BITA $0F with $F0: A unchanged               expect 0F C4
        ldaa    #$C0
        tap
        ldaa    #$0F
        bita    #$F0
        jsr     save
; MUL $12*$34 = $03A8: C is bit 7 of B         expect 03 A8 C1
        ldaa    #$C0
        tap
        ldaa    #$12
        ldab    #$34
        mul
        jsr     save16
; ADDD $7FFF+$0001: overflow                   expect 80 00 CA
        ldaa    #$C0
        tap
        ldd     #$7FFF
        addd    #$0001
        jsr     save16
; SUBD $0000-$0001: negative, borrow           expect FF FF C9
        ldaa    #$C0
        tap
        ldd     #$0000
        subd    #$0001
        jsr     save16
; LSRD $0003                                   expect 00 01 C3
        ldaa    #$C0
        tap
        ldd     #$0003
        lsrd
        jsr     save16
; ASLD $8001                                   expect 00 02 C3
        ldaa    #$C0
        tap
        ldd     #$8001
        asld
        jsr     save16
; CPX $1000 with $1001: the 6801 sets C        expect 10 00 C9
        ldaa    #$C0
        tap
        pshx
        ldx     #$1000
        cpx     #$1001
        tpa
        pulx
        psha
        ldd     #$1000
        std     0,x
        pula
        staa    2,x
        inx
        inx
        inx
; ABX $10F0+$20                                expect 11 10
        pshx
        ldx     #$10F0
        ldab    #$20
        abx
        stx     $00
        pulx
        ldd     $00
        std     0,x
        inx
        inx
; INC, CLR, COM and TST on memory              expect 02 FF 00
        clr     0,x
        inc     0,x
        inc     0,x
        clr     1,x
        com     1,x
        clr     2,x
        tst     2,x
        bra     *

; store A and then the condition codes at X, and advance X
save    psha
        tpa
        staa    1,x
        pula
        staa    0,x
        inx
        inx
        rts

; store D and then the condition codes at X, and advance X
save16  pshb
        psha
        tpa
        staa    2,x
        pula
        pulb
        std     0,x
        inx
        inx
        inx
        rts
