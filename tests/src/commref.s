; commref.s - shared imported (under its counter, 3), with no common
; block of its own: only libcomm.a's member has one, so the member is
; linked for it
	LOC	0
	ENTRY	start
start	ldx	shared
	rts
	LOC	3
	IMPORT	shared
