; noload.s - initialized data in counter 4, and only reserved space in
; counter 5, for the NOLOAD tests
	LOC	0
	ENTRY	nlstart
nlstart	rts
	LOC	4
init	fcb	1		; initialized: lost in a NOLOAD section
	fdb	init		; and a relocation
	LOC	5
	rmb	4		; only reserved: fine in a NOLOAD section
