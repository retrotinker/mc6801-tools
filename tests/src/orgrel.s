; orgrel.s - ORG and BLOCK in a relocatable module are errors: the linker
; places the module, so it can't have code at a fixed address
	LOC	0
start	nop
	ORG	$100
	nop
	BLOCK	$200
	nop
	ENDB
