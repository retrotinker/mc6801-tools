/* noload-ok.ld - only reserved space in the NOLOAD section: it links,
   and counter 5 is uninitialized data */
SECTIONS
{
    .text 0x1000 : { *(LOC0 LOC1) }
    .data : { *(LOC2 LOC3 LOC4) }
    .bss (NOLOAD) : { *(LOC5) *(COMMON) }
}
