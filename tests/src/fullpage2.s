; fullpage2.s - fullpage.s with a byte more in location counter 2, which
; then crosses into the next page: the linker moves it there
	LOC	0
	ENTRY	fstart
fstart	ldx	#dpvar
	rts

	LOC	2
	export	dpvar
dpvar	rmb	$80
	fcb	1
