; dpage.s - DPAGE: references to page-zero data in location counter 5
;
; Linked with dpshared.s and dpage.ld, which put counter 5 at $80; run on
; the simulator, the values it loads show that each reference reached
; the right byte.  The comments give the expected opcodes.
	DPAGE	5
	LOC	5
	IMPORT	shared		; another module's page-zero variable
count	fcb	$11		; this module's: $80
	fcb	$22
result	RMB	2
flags	COMM	1		; a common block
	LOC	0
	ENTRY	dpstart
dpstart	lds	#$7F
	ldaa	count		; direct: 96
	ldab	count+1		; direct, an expression: D6
	std	result		; direct: DD
	ldaa	shared		; direct, imported: 96
	staa	flags		; direct, common: 97
	ldab	>count		; extended, forced: F6
	inc	count		; extended, no direct INC: 7C
	ldx	#count		; 16-bit immediate: CE
	ldaa	#count		; 8-bit immediate: 86
	ldab	#shared+1	; 8-bit immediate, imported: C6
	stx	later		; direct, a forward reference: DF
	ldaa	#later		; 8-bit immediate, a forward reference: 86
	ldx	#table
	bra	*
table	fcb	count, shared, later, 7	; three addresses and a number
	fdb	count
	LOC	5
later	RMB	2
