/* Default linker script for ld01.
   Values from the command line: -T 0x1000, no -D, no -H. */

OUTPUT_FORMAT(aout)

SECTIONS
{
    .text 0x1000 :
    {
        __btext = .;
        PROVIDE(__segoff = 0);
        PROVIDE(__seg0DL = .);
        *(LOC0)
        PROVIDE(__seg0DH = .);
        PROVIDE(__seg1DL = .);
        *(LOC1)
        PROVIDE(__seg1DH = .);
        __etext = .;
        . = ALIGN(16);
    }

    .data 0x0000 :
    {
        __bdata = .;
        ONEPAGE
        {
            PROVIDE(__seg2DL = .);
            *(LOC2)
            PROVIDE(__seg2DH = .);
        }
        PROVIDE(__seg3DL = .);
        *(LOC3)
        PROVIDE(__seg3DH = .);
        PROVIDE(__seg5DL = .);
        *(LOC5)
        PROVIDE(__seg5DH = .);
        PROVIDE(__seg6DL = .);
        *(LOC6)
        PROVIDE(__seg6DH = .);
        PROVIDE(__seg7DL = .);
        *(LOC7)
        PROVIDE(__seg7DH = .);
        PROVIDE(__seg8DL = .);
        *(LOC8)
        PROVIDE(__seg8DH = .);
        PROVIDE(__seg9DL = .);
        *(LOC9)
        PROVIDE(__seg9DH = .);
        PROVIDE(__segADL = .);
        *(LOC10)
        PROVIDE(__segADH = .);
        PROVIDE(__segBDL = .);
        *(LOC11)
        PROVIDE(__segBDH = .);
        PROVIDE(__segCDL = .);
        *(LOC12)
        PROVIDE(__segCDH = .);
        PROVIDE(__segDDL = .);
        *(LOC13)
        PROVIDE(__segDDH = .);
        PROVIDE(__segEDL = .);
        *(LOC14)
        PROVIDE(__segEDH = .);
        PROVIDE(__segFDL = .);
        *(LOC15)
        PROVIDE(__segFDH = .);
        __edata = .;
    }

    .bss (NOLOAD) :
    {
        PROVIDE(__seg4DL = .);
        *(LOC4)
        PROVIDE(__seg4DH = .);
        PROVIDE(__seg0CL = .);
        *(COMMON0)
        PROVIDE(__seg0CH = .);
        PROVIDE(__seg1CL = .);
        *(COMMON1)
        PROVIDE(__seg1CH = .);
        PROVIDE(__seg2CL = .);
        *(COMMON2)
        PROVIDE(__seg2CH = .);
        PROVIDE(__seg3CL = .);
        *(COMMON3)
        PROVIDE(__seg3CH = .);
        PROVIDE(__seg4CL = .);
        *(COMMON4)
        PROVIDE(__seg4CH = .);
        PROVIDE(__seg5CL = .);
        *(COMMON5)
        PROVIDE(__seg5CH = .);
        PROVIDE(__seg6CL = .);
        *(COMMON6)
        PROVIDE(__seg6CH = .);
        PROVIDE(__seg7CL = .);
        *(COMMON7)
        PROVIDE(__seg7CH = .);
        PROVIDE(__seg8CL = .);
        *(COMMON8)
        PROVIDE(__seg8CH = .);
        PROVIDE(__seg9CL = .);
        *(COMMON9)
        PROVIDE(__seg9CH = .);
        PROVIDE(__segACL = .);
        *(COMMON10)
        PROVIDE(__segACH = .);
        PROVIDE(__segBCL = .);
        *(COMMON11)
        PROVIDE(__segBCH = .);
        PROVIDE(__segCCL = .);
        *(COMMON12)
        PROVIDE(__segCCH = .);
        PROVIDE(__segDCL = .);
        *(COMMON13)
        PROVIDE(__segDCH = .);
        PROVIDE(__segECL = .);
        *(COMMON14)
        PROVIDE(__segECH = .);
        PROVIDE(__segFCL = .);
        *(COMMON15)
        PROVIDE(__segFCH = .);
        __end = .;
    }

    PROVIDE(__heap_top = __end + 0x0000);
}
