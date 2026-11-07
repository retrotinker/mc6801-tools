; bssinit.s - initialized data under .BSS: with the default layout, which
; doesn't write .BSS, a link error
	LOC	0
	ENTRY	binit
binit	rts
	.BSS
bad	fcb	1
