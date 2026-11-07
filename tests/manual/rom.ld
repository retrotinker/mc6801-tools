/* rom.ld - a program in an 8 KB EPROM at $E000, with its variables in
   the MC6801's internal RAM */
ENTRY(reset)
OUTPUT_FORMAT(srec)
FILL(0xFF)

MEMORY
{
    ROM (rx) : ORIGIN = 0xE000, LENGTH = 0x1FF0
    VEC (r)  : ORIGIN = 0xFFF0, LENGTH = 16
    RAM (rw) : ORIGIN = 0x0080, LENGTH = 128
}

SECTIONS
{
    .vectors :
    {
        KEEP(vectors.o(LOC0))
    } > VEC

    .text :
    {
        crt0.o(LOC0)
        *(LOC0 LOC1)
    } > ROM

    .data :
    {
        *(LOC2 LOC3 LOC5 LOC6 LOC7 LOC8 LOC9 LOC1?)
    } > RAM AT> ROM

    .bss (NOLOAD) :
    {
        *(.bss)
        *(COMMON)
    } > RAM

    __stack_top = ORIGIN(RAM) + LENGTH(RAM) - 1;

    ASSERT(SIZEOF(.vectors) == 16, "the vector table must be 16 bytes")
}
