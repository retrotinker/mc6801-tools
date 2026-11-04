/* custom.ld - a linker script test: puts helper.o's code first, starts
   the data on the next 256-byte page, and defines symbols */
ENTRY(ustart)

SECTIONS
{
    .text 0x4000 :
    {
        helper.o(LOC0)
        *(LOC0 LOC1)
        *(COMMON0 COMMON1)
        text_end = .;
    }

    . = ALIGN(0x100);

    .data :
    {
        *(LOC2 LOC3 LOC4 LOC5 LOC6 LOC7 LOC8 LOC9)
        *(LOC1?)                /* LOC10 to LOC15 */
        HIDDEN(scratch = .);
        . = scratch + 2;        # a gap
        *(COMMON)
    }

    stacktop = ADDR(.data) + SIZEOF(.data) + 0x40;
    ioport = 0x0002;
    PROVIDE(__heap_top = 0x7000);
    PROVIDE(unused = MAX(1, 2));
}
