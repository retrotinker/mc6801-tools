; crt0.s - start-up code for a ROM program: sets up the stack, copies
; the initialized data from ROM to RAM, and calls main
        LOC     0
        ENTRY   reset
        EXPORT  ignore
        IMPORT  __stack_top, __data_load, __data_start, __data_end, main
reset   lds     #__stack_top
        ldx     #__data_start   ; any data to copy?
        cpx     #__data_end
        beq     run
        ldx     #__data_load    ; copy it from ROM to RAM
        stx     src
        ldx     #__data_start
        stx     dst
copy    ldx     src
        ldaa    0,x
        inx
        stx     src
        ldx     dst
        staa    0,x
        inx
        stx     dst
        cpx     #__data_end
        bne     copy
run     jsr     main
        bra     *

ignore  rti                     ; for interrupts that aren't used

        LOC     3
src     COMM    2               ; copy pointers
dst     COMM    2
