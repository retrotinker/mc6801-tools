; allops.s - every MC6801 instruction in every addressing mode
;
; The regression tests assemble this and check that the simulator
; (sim6801 -L) decodes the same instruction at every address as the
; assembler's listing shows, so the simulator's instruction table and
; lengths match as01.  Generated with as01, in the way that
; doc/tools/gen-opcodes.py finds the opcodes; synonyms (LSL, LSLD, BHS,
; BLO, CMPX) are left out, as they assemble to the same opcodes.
	nop
	lsrd
	asld
	tap
	tpa
	inx
	dex
	clv
	sev
	clc
	sec
	cli
	sei
	sba
	cba
	tab
	tba
	daa
	aba
	bra	*
	brn	*
	bhi	*
	bls	*
	bcc	*
	bcs	*
	bne	*
	beq	*
	bvc	*
	bvs	*
	bpl	*
	bmi	*
	bge	*
	blt	*
	bgt	*
	ble	*
	tsx
	ins
	pula
	pulb
	des
	txs
	psha
	pshb
	pulx
	rts
	abx
	rti
	pshx
	mul
	wai
	swi
	nega
	coma
	lsra
	rora
	asra
	asla
	rola
	deca
	inca
	tsta
	clra
	negb
	comb
	lsrb
	rorb
	asrb
	aslb
	rolb
	decb
	incb
	tstb
	clrb
	neg	0,x
	com	0,x
	lsr	0,x
	ror	0,x
	asr	0,x
	asl	0,x
	rol	0,x
	dec	0,x
	inc	0,x
	tst	0,x
	jmp	0,x
	clr	0,x
	neg	>$1234
	com	>$1234
	lsr	>$1234
	ror	>$1234
	asr	>$1234
	asl	>$1234
	rol	>$1234
	dec	>$1234
	inc	>$1234
	tst	>$1234
	jmp	>$1234
	clr	>$1234
	suba	#1
	cmpa	#1
	sbca	#1
	subd	#1
	anda	#1
	bita	#1
	ldaa	#1
	eora	#1
	adca	#1
	oraa	#1
	adda	#1
	cpx	#1
	bsr	*
	lds	#1
	suba	<$80
	cmpa	<$80
	sbca	<$80
	subd	<$80
	anda	<$80
	bita	<$80
	ldaa	<$80
	staa	<$80
	eora	<$80
	adca	<$80
	oraa	<$80
	adda	<$80
	cpx	<$80
	jsr	<$80
	lds	<$80
	sts	<$80
	suba	0,x
	cmpa	0,x
	sbca	0,x
	subd	0,x
	anda	0,x
	bita	0,x
	ldaa	0,x
	staa	0,x
	eora	0,x
	adca	0,x
	oraa	0,x
	adda	0,x
	cpx	0,x
	jsr	0,x
	lds	0,x
	sts	0,x
	suba	>$1234
	cmpa	>$1234
	sbca	>$1234
	subd	>$1234
	anda	>$1234
	bita	>$1234
	ldaa	>$1234
	staa	>$1234
	eora	>$1234
	adca	>$1234
	oraa	>$1234
	adda	>$1234
	cpx	>$1234
	jsr	>$1234
	lds	>$1234
	sts	>$1234
	subb	#1
	cmpb	#1
	sbcb	#1
	addd	#1
	andb	#1
	bitb	#1
	ldab	#1
	eorb	#1
	adcb	#1
	orab	#1
	addb	#1
	ldd	#1
	ldx	#1
	subb	<$80
	cmpb	<$80
	sbcb	<$80
	addd	<$80
	andb	<$80
	bitb	<$80
	ldab	<$80
	stab	<$80
	eorb	<$80
	adcb	<$80
	orab	<$80
	addb	<$80
	ldd	<$80
	std	<$80
	ldx	<$80
	stx	<$80
	subb	0,x
	cmpb	0,x
	sbcb	0,x
	addd	0,x
	andb	0,x
	bitb	0,x
	ldab	0,x
	stab	0,x
	eorb	0,x
	adcb	0,x
	orab	0,x
	addb	0,x
	ldd	0,x
	std	0,x
	ldx	0,x
	stx	0,x
	subb	>$1234
	cmpb	>$1234
	sbcb	>$1234
	addd	>$1234
	andb	>$1234
	bitb	>$1234
	ldab	>$1234
	stab	>$1234
	eorb	>$1234
	adcb	>$1234
	orab	>$1234
	addb	>$1234
	ldd	>$1234
	std	>$1234
	ldx	>$1234
	stx	>$1234
