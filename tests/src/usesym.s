; usesym.s - refers to symbols that linker scripts define, from two
; location counters
	LOC	3
	IMPORT	ioport
	LOC	0
	ENTRY	ustart
	IMPORT	stacktop, __heap_top
ustart	lds	#stacktop
	ldx	#__heap_top
	ldaa	ioport
	rts
	LOC	3
ubyte	fcb	1
