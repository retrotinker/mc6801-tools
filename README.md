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

********************************************************************************

# [Expanded Tool Demo] (https://mc6801-tools.blogspot.com/2024/01/expanded-tool-demo.html)
   -- January 19, 2024

So, we have demonstrated that this toolchain can produce a simple
6801 binary that will run on the MC-10. But honestly, that was
already available with TASM or other "absolute" assemblers available
to the community. Surely there is something more to consider with
this toolchain?

## Modules

The primary advantage that I see with this toolchain is the ability
to build object modules. This requires a few modifications to the
source code, but allows for dividing the project in a modular way.

Let's start by moving clrscn and prtstring to a separate source file
(screen.s):

    LOC 0

    SCREEN  equ     $4000

    clrscn  ldx     #SCREEN
            ldaa    #$40+'
            ldab    #$40+'
    clrsc.1 std     ,x
            inx
            inx
            cpx     #(SCREEN+512)
            blt     clrsc.1

            rts

            EXPORT  clrscn

    prstring
            tsx
            ldx     2,x
    prstlop ldaa    ,x
            beq     prstxit
            inx
            pshx
            ldx     $ffde
            jsr     ,x
            pulx
            bra     prstlop
    prstxit rts

            EXPORT  prstring

Notice the  EXPORT assembler directives used to make subroutine entry
points available for linking!

Along with moving the code itself, there are corresponding changes
to main.s:

    LOC 0

            IMPORT  clrscn
            IMPORT  prstring

            ENTRY __start

    msg     fcb     'M,'E,'S,'S,'A,'G,'E',$00

    __start
            jsr     clrscn

            ldx     #msg
            pshx
            jsr     prstring
            ins
            ins

            rts

Now notice the IMPORT assembler directives used to identify the
externally defined subroutines. Also note that prstring now requires a
pointer to the string for display, passed as a parameter on the stack.

These two source files are then assembled to two different object
modules:

    as01 -o screen.o screen.s

    as01 -o main.o main.s

And these two modules are linked to build the a.out, which is then
converted to a WAV file:

    ld01 -T0x434c -o main.aout main.o screen.o

    objcopy01 -T0x434c -O wav main.aout main.wav

This is sufficient to produce a binary that loads and prints a message
to the MC-10 screen.

## Libraries

One of the main benefits of building code into object modules is that
this provides for packaging code into libraries of objects that can
then be used  to build future programs. Let's expand on that idea
with modules for multiple messages.

Let's start with one.s:

    LOC 0

        IMPORT    prstring

    msg_one    fcb    'O,'N,'E,$0d,$00

    one
        ldx    #msg_one
        pshx
        jsr    prstring
        ins
        ins

        rts

        EXPORT one

Continue with two.s:

    LOC 0

        IMPORT    prstring

    msg_two    fcb    'T,'W,'O,$0d,$00

    two
        ldx    #msg_two
        pshx
        jsr    prstring
        ins
        ins

        rts

        EXPORT two

And finish with three.s:

    LOC 0

        IMPORT    prstring

    msg_three    fcb    'T,'H,'R,'E,'E,$0d,$00

    three
        ldx    #msg_three
        pshx
        jsr    prstring
        ins
        ins

        rts

        EXPORT three

Finally, main.s now looks like this:

    LOC 0

        IMPORT    clrscn
        IMPORT    prstring

        IMPORT    one
        IMPORT    two
        IMPORT    three

        ENTRY __start

    __start
        jsr    clrscn

        jsr    one
        jsr    two
        jsr    three

        rts

We could simply link one.o, two.o, and three.o with main.o:

    ld01 -T0x434c -o main.aout main.o screen.o one.o two.o three.o

But instead, we could build those objects into a library, and link
from there:

    ar01 r libnum.a one.o two.o three.o

    ld01 -lnum -T0x434c -o main.aout main.o screen.o

Now, a library for printing number words seems of limited use. But
those with more software development experience should be able to
see the appeal.

## Runtime

One more feature of the toolchain (inherited from the toolchain's C
language heritage) is the provision for using different "C runtime"
modules. Normally this is the bit of code that is executed first in
a program, with responsibility for setting-up the stack and starting
the main routine. This seems like the obvious place to put the ENTRY
directive and to jump to the main program:

    LOC 0

        IMPORT    main

        ENTRY __start

    __start
        jsr    main

        rts

Save that as crtstart.s, and assemble that to crtstart.o. Modify
main.s to read as follows:

    LOC 0

        IMPORT    clrscn
        IMPORT    prstring

        IMPORT    one
        IMPORT    two
        IMPORT    three

    main
        jsr    clrscn

        jsr    one
        jsr    two
        jsr    three

        rts

        EXPORT    main

The following linker command now produces the a.out file, and the
objcopy01 command produces the final WAV file:

    ld01 -lnum -Cstart -T0x434c -o main.aout main.o screen.o
    objcopy01 -T0x434c -O wav main.aout main.wav

## Conclusion

So there it is: an assembler, linker, and archiver toolchain for the
Motorola 6801, and a demonstration of how to use the toolchain to
produce a program that will run on the Tandy MC-10 -- the gift you
never knew you wanted! Again, is it useful? That depends...
