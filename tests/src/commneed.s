; commneed.s - the common block shared, as commmain.s has it, and junk,
; which libcomm.a's member defines: the member is linked, and shared
; has one block of storage
	LOC	0
	ENTRY	start
	IMPORT	junk
start	ldx	shared
	jsr	junk
	rts
	LOC	3
shared	COMM	2
