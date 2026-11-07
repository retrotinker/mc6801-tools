/* dpzp.ld - dpref.s's data in page zero */
OUTPUT_FORMAT(srec)
SECTIONS
{
    .text 0xE000 : { *(LOC0 LOC1) }
    .data 0x0080 : { *(LOC2 LOC3) }
}
