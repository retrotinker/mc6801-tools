SECTIONS
{
    .text 0x1000 : { *(LOC0 LOC1) }
    .data 0x1004 : { *(LOC*) *(COMMON) }
}
