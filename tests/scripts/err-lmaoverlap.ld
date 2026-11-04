OUTPUT_FORMAT(binary)
SECTIONS
{
    .text 0x1000 : { *(LOC0 LOC1) *(COMMON0 COMMON1) }
    .data 0x8000 : AT(0x1010) { *(LOC*) *(COMMON) }
}
