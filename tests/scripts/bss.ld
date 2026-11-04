/* bss.ld - an a.out program with its common blocks in a NOLOAD
   section, which becomes the uninitialized data */
SECTIONS
{
    .text 0x1000 : { *(LOC0 LOC1) *(COMMON0 COMMON1) }
    .data        : { *(LOC*) }
    .bss (NOLOAD): { *(COMMON) }
}
