/* err-excl1.ld - EXCLUDE_FILE with no files */
SECTIONS
{
    .text 0x1000 : { EXCLUDE_FILE() *(LOC0 LOC1) }
}
