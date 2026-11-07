; dpref.s - one-byte references to page-zero data: a forced direct
; address of an imported symbol and of this module's own data, and an
; indexed offset; ld01 checks that each fits in a byte
	LOC	3
	IMPORT	counter
	LOC	0
	ENTRY	start
	EXPORT	start
start	ldaa	<counter
	ldab	<local
	ldx	#0
	ldaa	local+2,x
	bra	*
	LOC	3
local	fcb	$42
	fcb	0, $99
