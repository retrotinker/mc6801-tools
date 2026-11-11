; commmain.s - a common block, shared, which libcomm.a's member has too:
; that is no reason to link the member (nothing else in it is needed),
; and shared's storage comes with the program
	LOC	0
	ENTRY	start
start	ldx	shared
	rts
	LOC	3
shared	COMM	2
