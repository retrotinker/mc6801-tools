/* fullpage.ld: a ONEPAGE from $1080, which fullpage.s fills to the end */
SECTIONS
{
    .text 0x1000 : { *(LOC0) }
    .data 0x1080 : { ONEPAGE { *(LOC2) } *(LOC3) }
}
