LOC 0

	IMPORT	prstring

msg_one	fcb	'O,'N,'E,$0d,$00

one
	ldx	#msg_one
	pshx
	jsr	prstring
	ins
	ins

	rts

	EXPORT one
