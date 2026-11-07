; bssprog.s - uninitialized data (.BSS and a common block), cleared by
; the start-up code, and initialized data
	LOC	0
	ENTRY	bstart
	IMPORT	__bss_start, __bss_end
bstart	ldx	#__bss_start	; clear .BSS and the common blocks
clear	cpx	#__bss_end
	beq	cleared
	clr	0,x
	inx
	bra	clear
cleared	ldaa	#$11
	staa	var1
	ldd	#$2233
	std	table+2
	ldaa	init
	staa	copy
	bra	*

	.DATA
init	fcb	$44

	.BSS
var1	rmb	1
table	rmb	4
copy	COMM	1
