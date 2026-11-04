; segs.s - main module for the linker tests: uses several location
; counters and refers to symbols in other modules and counters
	LOC	0
	ENTRY	start
	IMPORT	helper, codecom
start	lds	#$00ff
	jsr	helper
	bsr	local
	ldx	#table
	ldaa	counter
	ldx	#codecom
	ldx	#message
	ldx	#fast
	ldx	#far5
	ldx	#last
	rts
local	nop
	rts

	LOC	1
fast	ldaa	#2
	rts

	LOC	3
	IMPORT	table, counter
message	fcc	"SEGS"
	fcb	0
	rmb	5
	fdb	start,local,fast

	LOC	5
far5	fcb	5,6,7

	LOC	15
last	fdb	message
	rmb	3
