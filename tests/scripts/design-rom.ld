/* design-rom.ld - the ROM program example from doc/design/ld01-linker-scripts.md,
   copied exactly */
ENTRY(reset)
OUTPUT_FORMAT(srec)

MEMORY
{
    ROM (rx) : ORIGIN = 0xE000, LENGTH = 0x1FF0
    VEC (r)  : ORIGIN = 0xFFF0, LENGTH = 16
    RAM (rw) : ORIGIN = 0x0080, LENGTH = 128
}

SECTIONS
{
    .text :
    {
        *(LOC0 LOC1)
    } > ROM = 0xFF

    .data :
    {
        __data_start = .;
        *(LOC3)
        __data_end = .;
    } > RAM AT> ROM

    .bss (NOLOAD) :
    {
        *(COMMON)
    } > RAM

    .vectors :
    {
        KEEP(vectors.o(LOC2))
    } > VEC

    __stack_top = ORIGIN(RAM) + LENGTH(RAM) - 1;
    __data_load = LOADADDR(.data);

    ASSERT(SIZEOF(.vectors) == 16, "vector table must be 16 bytes")
}
