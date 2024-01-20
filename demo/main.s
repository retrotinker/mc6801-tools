LOC 0

	IMPORT	clrscn
	IMPORT	prstring

	IMPORT	one
	IMPORT	two
	IMPORT	three

main
	jsr	clrscn

	jsr	one
	jsr	two
	jsr	three

	rts

	EXPORT	main
