; romcrt.s - start-up code for the ROM layout test: copies the
; initialized data from ROM to RAM, using symbols from the linker script
	LOC	3
	IMPORT	__data_start, __data_end
	LOC	0
	ENTRY	reset
	IMPORT	__data_load, __stack_top, start
reset	lds	#__stack_top
	ldx	#__data_load
	ldd	#__data_start
	ldd	#__data_end
	jsr	start
	bra	*

	LOC	3
vars	fcb	$11,$22,$33
