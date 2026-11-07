; zpmain.s - uses them
        DPAGE   5
        LOC     5
        IMPORT  count, flags    ; imported while counter 5 is current
        LOC     0
start   ldaa    count           ; 96 xx: direct
        staa    flags           ; 97 xx: direct
        ldab    >count          ; F6 xxxx: extended, forced with >
        inc     count           ; 7C xxxx: INC has no direct mode
        ldx     #count          ; CE xxxx: a 16-bit immediate
        ldab    #flags          ; C6 xx: the address as a byte
        ldaa    total           ; 96 xx: direct, though total comes later
        rts

        LOC     5
total   rmb     2
