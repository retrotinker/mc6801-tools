/* err-excl2.ld - EXCLUDE_FILE before an assignment */
SECTIONS
{
    .text 0x1000 : { EXCLUDE_FILE(helper.o) sym = 1; }
}
