/* Default linker script for ld01.
   Values from the command line: -T 0xE000, no -D, -H 0x0100. */

OUTPUT_FORMAT(aout)

SECTIONS
{
    .text 0xE000 : SUBALIGN(4)
    {
        __btext = .;
        PROVIDE(__segoff = 0);
        PROVIDE(__seg0DL = .);
        *(LOC0)
        PROVIDE(__seg0DH = .);
        PROVIDE(__seg0CL = .);
        *(COMMON0)
        PROVIDE(__seg0CH = .);
        PROVIDE(__seg1DL = .);
        *(LOC1)
        PROVIDE(__seg1DH = .);
        PROVIDE(__seg1CL = .);
        *(COMMON1)
        PROVIDE(__seg1CH = .);
        __etext = .;
    }

    .data __etext : SUBALIGN(4)
    {
        __bdata = .;
        HIDDEN(__dend = .);

        ONEPAGE
        {
            PROVIDE(__seg2DL = .);
            HIDDEN(__dpos = .);
            *(LOC2)
            HIDDEN(__dend = . != __dpos ? . : __dend);
            PROVIDE(__seg2DH = .);
            PROVIDE(__seg2CL = .);
            *(COMMON2)
            PROVIDE(__seg2CH = .);
        }

        PROVIDE(__seg3DL = .);
        HIDDEN(__dpos = .);
        *(LOC3)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__seg3DH = .);
        PROVIDE(__seg3CL = .);
        *(COMMON3)
        PROVIDE(__seg3CH = .);

        PROVIDE(__seg4DL = .);
        HIDDEN(__dpos = .);
        *(LOC4)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__seg4DH = .);
        PROVIDE(__seg4CL = .);
        *(COMMON4)
        PROVIDE(__seg4CH = .);

        PROVIDE(__seg5DL = .);
        HIDDEN(__dpos = .);
        *(LOC5)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__seg5DH = .);
        PROVIDE(__seg5CL = .);
        *(COMMON5)
        PROVIDE(__seg5CH = .);

        PROVIDE(__seg6DL = .);
        HIDDEN(__dpos = .);
        *(LOC6)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__seg6DH = .);
        PROVIDE(__seg6CL = .);
        *(COMMON6)
        PROVIDE(__seg6CH = .);

        PROVIDE(__seg7DL = .);
        HIDDEN(__dpos = .);
        *(LOC7)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__seg7DH = .);
        PROVIDE(__seg7CL = .);
        *(COMMON7)
        PROVIDE(__seg7CH = .);

        PROVIDE(__seg8DL = .);
        HIDDEN(__dpos = .);
        *(LOC8)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__seg8DH = .);
        PROVIDE(__seg8CL = .);
        *(COMMON8)
        PROVIDE(__seg8CH = .);

        PROVIDE(__seg9DL = .);
        HIDDEN(__dpos = .);
        *(LOC9)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__seg9DH = .);
        PROVIDE(__seg9CL = .);
        *(COMMON9)
        PROVIDE(__seg9CH = .);

        PROVIDE(__segADL = .);
        HIDDEN(__dpos = .);
        *(LOC10)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__segADH = .);
        PROVIDE(__segACL = .);
        *(COMMON10)
        PROVIDE(__segACH = .);

        PROVIDE(__segBDL = .);
        HIDDEN(__dpos = .);
        *(LOC11)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__segBDH = .);
        PROVIDE(__segBCL = .);
        *(COMMON11)
        PROVIDE(__segBCH = .);

        PROVIDE(__segCDL = .);
        HIDDEN(__dpos = .);
        *(LOC12)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__segCDH = .);
        PROVIDE(__segCCL = .);
        *(COMMON12)
        PROVIDE(__segCCH = .);

        PROVIDE(__segDDL = .);
        HIDDEN(__dpos = .);
        *(LOC13)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__segDDH = .);
        PROVIDE(__segDCL = .);
        *(COMMON13)
        PROVIDE(__segDCH = .);

        PROVIDE(__segEDL = .);
        HIDDEN(__dpos = .);
        *(LOC14)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__segEDH = .);
        PROVIDE(__segECL = .);
        *(COMMON14)
        PROVIDE(__segECH = .);

        PROVIDE(__segFDL = .);
        HIDDEN(__dpos = .);
        *(LOC15)
        HIDDEN(__dend = . != __dpos ? . : __dend);
        PROVIDE(__segFDH = .);
        PROVIDE(__segFCL = .);
        *(COMMON15)
        PROVIDE(__segFCH = .);

        __edata = __dend;
        __end = .;
    }

    PROVIDE(__heap_top = __end + 0x0100);
}
