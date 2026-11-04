; romvec.s - the vector table for the ROM layout test: nothing refers to
; it, so the linker script has to KEEP it
	LOC	0
	IMPORT	reset
	fdb	reset,reset,reset,reset,reset,reset,reset,reset
