/* mc10.ld - a program for the Tandy MC-10, loaded from cassette at
   $434C: the start-up code first, then the rest of the code, then the
   data */
ENTRY(__start)

SECTIONS
{
    .text 0x434C :
    {
        __btext = .;
        crtstart.o(LOC0)
        *(LOC0 LOC1)
        *(COMMON0 COMMON1)
        __etext = .;
    }

    .data :
    {
        __bdata = .;
        *(LOC*)
        *(COMMON)
    }

    __stack_top = 0x4FFF;
}
