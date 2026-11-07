; branch.s - the conditional branches after comparisons
;
; For each comparison, every branch instruction is tried, and $01 (taken)
; or $00 (not taken) is stored in RAM from $80 on, one byte per branch in
; the order BRA BRN BHI BLS BCC BCS BNE BEQ BVC BVS BPL BMI BGE BLT BGT
; BLE.  The comparisons make signed and unsigned results differ, and the
; "expect" comments follow from what each branch means: BHI, BLS, BCC and
; BCS compare unsigned, BGE, BLT, BGT and BLE signed.

        ORG     $1000
start   lds     #$00FF
        ldx     #$80
; $80 vs $01: unsigned 128 > 1, signed -128 < 1
;   expect 01 00 01 00 01 00 01 00 00 01 01 00 00 01 00 01
        ldaa    #$80
        cmpa    #$01
        bra     y1
        clrb
        bra     s1
y1      ldab    #1
s1      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        brn     y2
        clrb
        bra     s2
y2      ldab    #1
s2      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        bhi     y3
        clrb
        bra     s3
y3      ldab    #1
s3      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        bls     y4
        clrb
        bra     s4
y4      ldab    #1
s4      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        bcc     y5
        clrb
        bra     s5
y5      ldab    #1
s5      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        bcs     y6
        clrb
        bra     s6
y6      ldab    #1
s6      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        bne     y7
        clrb
        bra     s7
y7      ldab    #1
s7      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        beq     y8
        clrb
        bra     s8
y8      ldab    #1
s8      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        bvc     y9
        clrb
        bra     s9
y9      ldab    #1
s9      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        bvs     y10
        clrb
        bra     s10
y10      ldab    #1
s10      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        bpl     y11
        clrb
        bra     s11
y11      ldab    #1
s11      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        bmi     y12
        clrb
        bra     s12
y12      ldab    #1
s12      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        bge     y13
        clrb
        bra     s13
y13      ldab    #1
s13      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        blt     y14
        clrb
        bra     s14
y14      ldab    #1
s14      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        bgt     y15
        clrb
        bra     s15
y15      ldab    #1
s15      stab    0,x
        inx
        ldaa    #$80
        cmpa    #$01
        ble     y16
        clrb
        bra     s16
y16      ldab    #1
s16      stab    0,x
        inx
; $01 vs $80: unsigned 1 < 128, signed 1 > -128
;   expect 01 00 00 01 00 01 01 00 00 01 00 01 01 00 01 00
        ldaa    #$01
        cmpa    #$80
        bra     y17
        clrb
        bra     s17
y17      ldab    #1
s17      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        brn     y18
        clrb
        bra     s18
y18      ldab    #1
s18      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        bhi     y19
        clrb
        bra     s19
y19      ldab    #1
s19      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        bls     y20
        clrb
        bra     s20
y20      ldab    #1
s20      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        bcc     y21
        clrb
        bra     s21
y21      ldab    #1
s21      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        bcs     y22
        clrb
        bra     s22
y22      ldab    #1
s22      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        bne     y23
        clrb
        bra     s23
y23      ldab    #1
s23      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        beq     y24
        clrb
        bra     s24
y24      ldab    #1
s24      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        bvc     y25
        clrb
        bra     s25
y25      ldab    #1
s25      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        bvs     y26
        clrb
        bra     s26
y26      ldab    #1
s26      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        bpl     y27
        clrb
        bra     s27
y27      ldab    #1
s27      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        bmi     y28
        clrb
        bra     s28
y28      ldab    #1
s28      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        bge     y29
        clrb
        bra     s29
y29      ldab    #1
s29      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        blt     y30
        clrb
        bra     s30
y30      ldab    #1
s30      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        bgt     y31
        clrb
        bra     s31
y31      ldab    #1
s31      stab    0,x
        inx
        ldaa    #$01
        cmpa    #$80
        ble     y32
        clrb
        bra     s32
y32      ldab    #1
s32      stab    0,x
        inx
; $05 vs $05: equal
;   expect 01 00 00 01 01 00 00 01 01 00 01 00 01 00 00 01
        ldaa    #$05
        cmpa    #$05
        bra     y33
        clrb
        bra     s33
y33      ldab    #1
s33      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        brn     y34
        clrb
        bra     s34
y34      ldab    #1
s34      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        bhi     y35
        clrb
        bra     s35
y35      ldab    #1
s35      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        bls     y36
        clrb
        bra     s36
y36      ldab    #1
s36      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        bcc     y37
        clrb
        bra     s37
y37      ldab    #1
s37      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        bcs     y38
        clrb
        bra     s38
y38      ldab    #1
s38      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        bne     y39
        clrb
        bra     s39
y39      ldab    #1
s39      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        beq     y40
        clrb
        bra     s40
y40      ldab    #1
s40      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        bvc     y41
        clrb
        bra     s41
y41      ldab    #1
s41      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        bvs     y42
        clrb
        bra     s42
y42      ldab    #1
s42      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        bpl     y43
        clrb
        bra     s43
y43      ldab    #1
s43      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        bmi     y44
        clrb
        bra     s44
y44      ldab    #1
s44      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        bge     y45
        clrb
        bra     s45
y45      ldab    #1
s45      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        blt     y46
        clrb
        bra     s46
y46      ldab    #1
s46      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        bgt     y47
        clrb
        bra     s47
y47      ldab    #1
s47      stab    0,x
        inx
        ldaa    #$05
        cmpa    #$05
        ble     y48
        clrb
        bra     s48
y48      ldab    #1
s48      stab    0,x
        inx
; $7F vs $FF: unsigned 127 < 255, signed 127 > -1, overflow
;   expect 01 00 00 01 00 01 01 00 00 01 00 01 01 00 01 00
        ldaa    #$7F
        cmpa    #$FF
        bra     y49
        clrb
        bra     s49
y49      ldab    #1
s49      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        brn     y50
        clrb
        bra     s50
y50      ldab    #1
s50      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        bhi     y51
        clrb
        bra     s51
y51      ldab    #1
s51      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        bls     y52
        clrb
        bra     s52
y52      ldab    #1
s52      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        bcc     y53
        clrb
        bra     s53
y53      ldab    #1
s53      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        bcs     y54
        clrb
        bra     s54
y54      ldab    #1
s54      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        bne     y55
        clrb
        bra     s55
y55      ldab    #1
s55      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        beq     y56
        clrb
        bra     s56
y56      ldab    #1
s56      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        bvc     y57
        clrb
        bra     s57
y57      ldab    #1
s57      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        bvs     y58
        clrb
        bra     s58
y58      ldab    #1
s58      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        bpl     y59
        clrb
        bra     s59
y59      ldab    #1
s59      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        bmi     y60
        clrb
        bra     s60
y60      ldab    #1
s60      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        bge     y61
        clrb
        bra     s61
y61      ldab    #1
s61      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        blt     y62
        clrb
        bra     s62
y62      ldab    #1
s62      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        bgt     y63
        clrb
        bra     s63
y63      ldab    #1
s63      stab    0,x
        inx
        ldaa    #$7F
        cmpa    #$FF
        ble     y64
        clrb
        bra     s64
y64      ldab    #1
s64      stab    0,x
        inx
        bra     *
