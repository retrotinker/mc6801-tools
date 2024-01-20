LOC 0

	IMPORT	prstring

msg_two	fcb	'T,'W,'O,$0d,$00

two
	ldx	#msg_two
	pshx
	jsr	prstring
	ins
	ins

	rts

	EXPORT two
