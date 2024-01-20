LOC 0

SCREEN	equ	$4000

clrscn	ldx	#SCREEN
	ldaa	#$40+' 
	ldab	#$40+' 
clrsc.1	std	,x
	inx
	inx
	cpx	#(SCREEN+512)
	blt	clrsc.1

	rts

	EXPORT	clrscn

prstring
	tsx
	ldx	2,x
prstlop	ldaa	,x
	beq	prstxit
	inx
	pshx
	ldx	$ffde
	jsr	,x
	pulx
	bra	prstlop
prstxit rts

	EXPORT	prstring
