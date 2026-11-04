; helper.s - second module for the linker tests: code, data and common
; blocks in several location counters
	LOC	0
	EXPORT	helper
	IMPORT	start
helper	ldaa	#1
	bsr	help2
	rts
help2	rts
codecom	COMM	4

	LOC	3
	EXPORT	table
table	fdb	helper,start
	fcb	1,2,3
counter	COMM	2
buf	COMM	16
