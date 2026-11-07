/* bss-pat.ld - the default layout for bssprog.o, using .bss in a
   pattern: the output is the same as without a script */
SECTIONS
{
    .text 0x1000 : { __btext = .; *(LOC0 LOC1) __etext = .; }
    .data : { __bdata = .; *(LOC2 LOC3 LOC5 LOC6 LOC7 LOC8 LOC9 LOC1?) __edata = .; }
    .bss (NOLOAD) : { *(.bss) *(COMMON) __end = .; }
}
