/* rom-excl.ld - rom.ld with .vectors after .text: EXCLUDE_FILE keeps
   .text's patterns from taking the vector table, so the order of the
   sections doesn't matter, and the output is the same as rom.ld's */
ENTRY(reset)
OUTPUT_FORMAT(binary)
FILL(0xFF)
INCLUDE "rom-memory.ld"

SECTIONS
{
    .text :
    {
        romcrt.o(LOC0)
        EXCLUDE_FILE(romvec.o) *(LOC0 LOC1)
        . = ALIGN(4);
        BYTE(0x42)
        WORD(0x1234)
        FILL(0xEE)
        . = . + 3;
    } > ROM

    .data :
    {
        __data_start = .;
        *(LOC2 LOC3 LOC4 LOC5 LOC6 LOC7 LOC8 LOC9 LOC1?)
        . = ALIGN(8);
        __data_end = .;
    } > RAM AT> ROM = 0xAA

    .bss (NOLOAD) :
    {
        *(COMMON)
    } > RAM

    .vectors :
    {
        KEEP(EXCLUDE_FILE(romcrt.o) rom*.o(LOC0))
    } > VEC

    __stack_top = ORIGIN(RAM) + LENGTH(RAM) - 1;
    ASSERT(SIZEOF(.vectors) == 16, "the vector table must be 16 bytes")
}
