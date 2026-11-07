; dperr.s - forced direct addresses that don't fit in a byte
	LOC	0
	ldaa	<$1234		; an absolute address
	ldaa	<far		; $123 bytes into this module's counter 3
	ldaa	<$80		; fits
	LOC	3
	RMB	$123
far	fcb	1
