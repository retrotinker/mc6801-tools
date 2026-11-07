; prog.s - a tiny program: shows a changing pattern on port 1
        DPAGE   3
PORT1   equ     $02
        LOC     3
pattern fcb     $55
count   COMM    1
        LOC     0
        EXPORT  main
main    ldaa    pattern
loop    staa    PORT1
        coma
        inc     count
        bra     loop
