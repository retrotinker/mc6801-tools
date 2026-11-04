; crt0.s - start-up code for the design document's ROM example
	LOC	3
	IMPORT	__data_start, __data_end
	LOC	0
	ENTRY	reset
	IMPORT	__data_load, __stack_top
reset	lds	#__stack_top
	ldx	#__data_load
	ldd	#__data_start
	ldd	#__data_end
	bra	*

	LOC	3
greeting fcc	"HI"
