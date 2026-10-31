
\normalsize

# File Formats {#a.out-format}

## Object files

`as01` writes object files in the Introl linkable binary format,
described in the *Introl Linker and Loader Reference Manual*. They
start with the two bytes `S1` (`$53 $31`).

## Executables

`ld01` writes a.out executables. All multi-byte fields are big-endian.
The file starts with a 32-byte header:

| Offset | Size | Field | Contents |
|-------:|-----:|:------|:---------|
| 0 | 2 | magic | `$01 $03` |
| 2 | 1 | flags | `$10` executable; `$20` separate instruction and data (`-i`) |
| 3 | 1 | cpu | `$1F` (MC6801) |
| 4 | 1 | header length | `$20` (32) |
| 5 | 1 | unused | 0 |
| 6 | 2 | version | 0 |
| 8 | 4 | text | Size of the text segment |
| 12 | 4 | data | Size of the data segment |
| 16 | 4 | bss | Size of uninitialized data |
| 20 | 4 | entry | Entry point address |
| 24 | 4 | total | Total memory used |
| 28 | 4 | syms | Size of the symbol table |

The header is followed by the text segment, the data segment and the
symbol table. Each symbol table entry is 16 bytes: an 8-byte name
(padded with zeros, not necessarily terminated), a 4-byte value, a
1-byte storage class, a 1-byte auxiliary count and a 2-byte type.

`ld01 -d` writes only the text and data segments, with no header or
symbols.

# Command Reference

This appendix contains the manual pages for the tools, as installed in
`doc/man`.

