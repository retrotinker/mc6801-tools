/* err-noload.ld - initialized data in a NOLOAD section is an error
   (counter 4), but reserved space isn't (counter 5) */
SECTIONS
{
    .text 0x1000 : { *(LOC0 LOC1) }
    .data : { *(LOC2 LOC3) }
    .bss (NOLOAD) : { *(LOC4 LOC5) *(COMMON) }
}
