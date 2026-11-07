# mc6801-tools

Unix-style development tools for the Motorola MC6801 and MC6803
microprocessors, and for machines built around them such as the Tandy
MC-10: an assembler, a linker, a librarian, and tools to inspect and
convert the results.

| Tool | Does |
|:-----|:-----|
| `as01` | Assembles a source file into a relocatable object module, or an absolute binary image |
| `ld01` | Links object modules and libraries into a program, laid out by default or by a linker script |
| `ar01` | Builds and maintains libraries of object modules |
| `objdump01` | Shows what's in object files, libraries and executables, including a disassembly; also installed as `nm01` and `size01` |
| `objcopy01` | Converts a program to S-records, Intel hex, a raw binary, or an MC-10 cassette file or audio |

## Features

* **Modules and libraries.** Split a program into modules that import
  and export symbols, and collect reusable modules into libraries.
* **Linker scripts**, a subset of GNU `ld`'s: place code and data in
  ROM and RAM, keep initialized data in ROM and copy it to RAM at
  start-up, put a vector table at `$FFF0`, define symbols, and write
  S-records or Intel hex directly.
* **Page zero.** Variables that a script puts in the MC6801's internal
  RAM get short direct addressing (`DPAGE`), and the linker checks that
  they really are in page zero.
* **Uninitialized data** (`.BSS`, common blocks) takes no space in the
  program file.
* **Tested by running.** `make check` compares over 500 outputs of the
  tools with references, and runs programs that the tools build on an
  MC6801 simulator, including cassette files decoded as the MC-10 would
  load them.

## Installing

Fedora packages are attached to each
[release](https://github.com/retrotinker/mc6801-tools/releases):
`mc6801-tools` (the tools and their man pages) and `mc6801-tools-doc`
(the user's manual and a demo program).

To build from source, you need a C compiler and `make`:

    make
    make check          # optional: the regression tests
    sudo make install

This installs the tools in `/usr/local/m6801-tools`, so add that to
your `PATH`, and the man pages in `/usr/local/share/man`. Set `PREFIX`
and `MANDIR` to install elsewhere.

## An example

This program prints a message on the MC-10's screen, using the output
routine in the MC-10's ROM:

    ; hello.s - print a message on the MC-10 screen
            LOC     0
            ENTRY   start

    start   ldx     #msg
    loop    ldaa    ,x              ; next character
            beq     done            ; zero byte ends the string
            inx
            pshx
            ldx     $ffde           ; ROM character output routine
            jsr     ,x
            pulx
            bra     loop
    done    rts

    msg     fcc     "HELLO, WORLD!"
            fcb     $0d,0

Assemble it, link it to load at `$434C`, and convert it to a cassette
audio file:

    as01 -o hello.o hello.s
    ld01 -T0x434c -o hello.aout hello.o
    objcopy01 -O wav -n HELLO hello.aout hello.wav

Play `hello.wav` into the MC-10's cassette input (or load it into an
emulator), type `CLOADM`, then `EXEC`. For other machines,
`objcopy01 -O srec` writes S-records instead.

The `demo` directory has a larger program, built from several modules
and a library.

## Documentation

* `doc/mc6801-tools-manual.pdf`: the user's manual, from a first
  program through modules, libraries and linker scripts to a complete
  ROM program, with every instruction's opcodes. (`make -C doc` rebuilds
  it, with `pandoc` and LaTeX.)
* `doc/man/`: the manual pages, installed by `make install`.
* `doc/design/`: the design documents for linker scripts, direct-page
  addressing and the 2.0 layout.
* `doc/history/`: the
  [blog post](doc/history/2024-01-expanded-tool-demo.md) that
  introduced the tools' modules, libraries and start-up code in 2024.

## Heritage

The tools are derived from the MC6809 configuration of the tools of
Bruce Evans' C compiler, as maintained in the
[dev86](https://github.com/lkundrak/dev86) package for Linux.

The assembler's syntax follows (and may be compatible with) section C.5
of the
[Introl-C Compiler Reference Manual](http://www.swtpc.com/mholley/Introl/REF.pdf).
Its object files are in the Introl linkable binary file format, described
in the
[Introl Linker and Loader Reference Manual](http://www.swtpc.com/mholley/Introl/LOAD.pdf).

## License

GNU General Public License, version 2 (see `COPYING`); a few files are
under the GPL version 2 or later, or the LGPL version 2 or later, as
their notices say.
