/* excl-orphan.ld - input that EXCLUDE_FILE keeps out of a section, and
   that no other statement takes, is an orphan: it goes after the last
   section that takes text from all files, not back where it was
   excluded */
SECTIONS
{
    .text 0x1000 : { EXCLUDE_FILE(helper.o) *(LOC0) }
    .text2 0x2000 : { *(LOC1) }
    .data 0x8000 : { *(LOC2 LOC3 LOC4 LOC5 LOC6 LOC7 LOC8 LOC9 LOC1?) *(COMMON) }
}
