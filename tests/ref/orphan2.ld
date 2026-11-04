/* orphan2.ld - orphans go to the last section of their kind that takes
   input from all files (.data), not to a later one that only takes a
   particular module's (.tail) */
SECTIONS
{
    .text 0 : { *(LOC0 LOC1) *(COMMON0 COMMON1) }
    .data   : { *(LOC3) *(COMMON3) }
    .tail   : { helper.o(LOC5) }
}
