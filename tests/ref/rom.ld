/* rom.ld - a ROM program: code in ROM, initialized data that runs in
   RAM but is stored in ROM, uninitialized data in RAM, and a vector
   table at $FFF0 */
ENTRY(reset)
OUTPUT_FORMAT(binary)
FILL(0xFF)
INCLUDE "rom-memory.ld"

SECTIONS
{
    /* first, so that its input isn't taken by .text's patterns */
    .vectors :
    {
        KEEP(romvec.o(LOC0))
    } > VEC

    .text :
    {
        romcrt.o(LOC0)
        *(LOC0 LOC1)
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

    __stack_top = ORIGIN(RAM) + LENGTH(RAM) - 1;
    ASSERT(SIZEOF(.vectors) == 16, "the vector table must be 16 bytes")
}
