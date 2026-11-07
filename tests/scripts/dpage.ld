/* dpage.ld - dpage.s with location counter 5 in page zero */
OUTPUT_FORMAT(srec)
ENTRY(dpstart)
SECTIONS
{
    .text 0xE000 : { *(LOC0 LOC1) }
    .zpage 0x0080 : { *(LOC5) *(COMMON5) }
}
