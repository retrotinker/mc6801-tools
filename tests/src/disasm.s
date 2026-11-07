; disasm.s - code with data in it, for objdump01 -d: an illegal opcode,
; a skipped area, a relocated word that isn't an operand, branches, and
; an instruction cut short at the end of the counter
	LOC	0
start	ldaa	#1
	fcb	$02		; not an instruction
	rmb	3		; skipped
	fdb	start		; a relocated word
loop	bra	start
	bsr	*
	ldaa	$80		; direct
	ldaa	>$80		; extended, forced
	fcb	$b6		; LDAA extended, cut short
	LOC	1
sub1	rts
