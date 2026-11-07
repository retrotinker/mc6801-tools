; dpagerr.s - DPAGE errors
	DPAGE	16		; no such location counter
here	DPAGE	4		; a label isn't allowed
	LOC	3
data3	fcb	1
	LOC	4
	RMB	$100
far	fcb	1		; 256 bytes into page-zero counter 4
	LOC	0
	ldaa	#data3		; relocatable, but not in a DPAGE counter
	fcb	data3
	ldaa	far		; too far into its counter
	ldaa	#far
	fcb	far
