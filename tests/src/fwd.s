; fwd.s - forward references: an address in page zero gets direct
; addressing even when it is defined, or imported, later; as01 makes
; more passes while instructions shrink and labels move
	DPAGE	5
	LOC	0
start	ldaa	PORT		; 96 02: an EQU defined later
	staa	RAMVAR		; 97 80
	ldx	PTR		; DE 82
	ldab	later		; D6 00: imported later, in page-zero counter 5
	ldaa	FAR		; B6 1234: not in page zero, so extended
	bra	start
PORT	equ	$02
RAMVAR	equ	$80
PTR	equ	$82
FAR	equ	$1234
	LOC	5
	IMPORT	later
