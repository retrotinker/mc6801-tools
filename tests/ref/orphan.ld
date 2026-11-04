/* orphan.ld - places only some of the input: the rest are orphans */
SECTIONS
{
    .text 0 : { *(LOC0 LOC1) *(COMMON0 COMMON1) }
    .data   : { *(LOC3) *(COMMON3) }
}
