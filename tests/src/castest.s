; castest.s - a program longer than a cassette block (255 bytes): it adds
; up its 600 data bytes, 200 each of $11, $22 and $33, so A ends as $B0
; (the low byte of 200 * $66)
	LOC	0
	ENTRY	cstart
cstart	ldx	#data
	clra
sum	adda	0,x
	inx
	cpx	#dend
	bne	sum
	bra	*
data	rmb	200,$11
	rmb	200,$22
	rmb	200,$33
dend
