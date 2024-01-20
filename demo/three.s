LOC 0

	IMPORT	prstring

msg_three	fcb	'T,'H,'R,'E,'E,$0d,$00

three
	ldx	#msg_three
	pshx
	jsr	prstring
	ins
	ins

	rts

	EXPORT three
