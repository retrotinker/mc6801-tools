; impuse.s - imports a variable under counter 0, though impdef.s defines
; it in counter 3, and a constant defined with EQU
	LOC	0
	ENTRY	impstart
	IMPORT	counter, SIZE
impstart ldaa	counter
	ldx	#SIZE
	rts
