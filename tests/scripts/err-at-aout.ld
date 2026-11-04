SECTIONS
{
    .text 0x1000 : { *(LOC0 LOC1) }
    .data : AT(0x2000) { *(LOC*) *(COMMON) }
}
