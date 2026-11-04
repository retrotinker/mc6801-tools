; vectors.s - the vector table for the design document's ROM example,
; in location counter 2 as the example expects
	LOC	0
	IMPORT	reset
	LOC	2
	fdb	reset,reset,reset,reset,reset,reset,reset,reset
