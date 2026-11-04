; vectors.s - the MC6801 interrupt vectors, for $FFF0 to $FFFF
        LOC     0
        IMPORT  reset, ignore
        fdb     ignore          ; SCI
        fdb     ignore          ; timer overflow
        fdb     ignore          ; output compare
        fdb     ignore          ; input capture
        fdb     ignore          ; IRQ1
        fdb     ignore          ; SWI
        fdb     ignore          ; NMI
        fdb     reset           ; reset
