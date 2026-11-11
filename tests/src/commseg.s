; commseg.s - shared imported under counter 0 (its counter isn't known
; here), with no common block of its own: libcomm.a's member has one, in
; counter 3, which wins whichever is read first, with a warning naming
; both counters (as for an import and a definition)
	LOC	0
	ENTRY	start
	IMPORT	shared
start	ldx	shared
	rts
