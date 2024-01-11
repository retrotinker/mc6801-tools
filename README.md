 Unix-like binutils for Motorola MC6801

 These cools are derived from the MC6809 configuration of the
 corresponding tools from the Bruce Evans C compiler, as maintained
 in the "dev86" package for Linux.

   https://github.com/lkundrak/dev86

The current version is capable of producing working code with a manual
sequence like the following:

	as01 -o start.o start.s
	as01 -o prog.o prog.s
	ld01 -d -o prog.bin -T0x1000 start.o prog.o
	objcopy --change-address=0x1000 -I binary -O srec prog.bin prog.s19

The resulting binary can be loaded onto your 6801-based target device
using whatever tools you have available to you.  Loading that code
and making it all work remains as an exercise for the reader...

Syntax for the assembler seems to be inspired by (and may be compatible
with) that described in section C.5 of "INTROL-C COMPILER REFERENCE
MANUAL", available here:

	http://www.swtpc.com/mholley/Introl/REF.pdf

The object file format produced by the assembler and consumed by the
linker is the "INTROL LINKABLE BINARY FILE FORMAT", documented in
"INTROL LINKER AND LOADER REFERENCE MANUAL", available here:

	http://www.swtpc.com/mholley/Introl/LOAD.pdf
