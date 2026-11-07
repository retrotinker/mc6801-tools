/* err-noload-byte.ld - BYTE in a NOLOAD section is an error */
SECTIONS
{
    .text 0x1000 : { *(LOC0 LOC1) }
    .data : { *(LOC2 LOC3 LOC4) }
    .bss (NOLOAD) : { BYTE(1) *(LOC5) }
}
