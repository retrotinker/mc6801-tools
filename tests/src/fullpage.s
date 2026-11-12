; fullpage.s - location counter 2 fills its page to the end (from $1080
; with fullpage.ld), so the linker leaves it there; fullpage2.s's is a
; byte longer and must move to the next page
	LOC	0
	ENTRY	fstart
fstart	ldx	#dpvar
	rts

	LOC	2
	export	dpvar
dpvar	rmb	$7f
	fcb	1
