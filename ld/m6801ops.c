/* m6801ops.c - the MC6801 instruction set: each opcode's mnemonic and
   addressing mode, for objdump01's disassembler and the tests' simulator
   (the regression tests check the table against as01) */

#include "m6801ops.h"

struct m6801_op m6801_ops[256];

static void setop(int code, const char *name, enum m6801_mode mode)
{
	m6801_ops[code].name = name;
	m6801_ops[code].mode = mode;
}

void m6801_init_ops(void)
{
	static const char *inh[0x40] = {
		[0x01] = "NOP", [0x04] = "LSRD", [0x05] = "ASLD",
		[0x06] = "TAP", [0x07] = "TPA", [0x08] = "INX",
		[0x09] = "DEX", [0x0A] = "CLV", [0x0B] = "SEV",
		[0x0C] = "CLC", [0x0D] = "SEC", [0x0E] = "CLI",
		[0x0F] = "SEI", [0x10] = "SBA", [0x11] = "CBA",
		[0x16] = "TAB", [0x17] = "TBA", [0x19] = "DAA",
		[0x1B] = "ABA", [0x30] = "TSX", [0x31] = "INS",
		[0x32] = "PULA", [0x33] = "PULB", [0x34] = "DES",
		[0x35] = "TXS", [0x36] = "PSHA", [0x37] = "PSHB",
		[0x38] = "PULX", [0x39] = "RTS", [0x3A] = "ABX",
		[0x3B] = "RTI", [0x3C] = "PSHX", [0x3D] = "MUL",
		[0x3E] = "WAI", [0x3F] = "SWI",
	};
	static const char *branch[16] = {
		"BRA", "BRN", "BHI", "BLS", "BCC", "BCS", "BNE", "BEQ",
		"BVC", "BVS", "BPL", "BMI", "BGE", "BLT", "BGT", "BLE",
	};
	/* $40-$7F: NEG..CLR on A, B, indexed and extended memory */
	static const char *rmw[16] = {
		"NEG", 0, 0, "COM", "LSR", 0, "ROR", "ASR",
		"ASL", "ROL", "DEC", 0, "INC", "TST", "JMP", "CLR",
	};
	static const char *rmwa[16] = {
		"NEGA", 0, 0, "COMA", "LSRA", 0, "RORA", "ASRA",
		"ASLA", "ROLA", "DECA", 0, "INCA", "TSTA", 0, "CLRA",
	};
	static const char *rmwb[16] = {
		"NEGB", 0, 0, "COMB", "LSRB", 0, "RORB", "ASRB",
		"ASLB", "ROLB", "DECB", 0, "INCB", "TSTB", 0, "CLRB",
	};
	/* $80-$BF and $C0-$FF, by the low four bits */
	static const char *aops[16] = {
		"SUBA", "CMPA", "SBCA", "SUBD", "ANDA", "BITA", "LDAA", "STAA",
		"EORA", "ADCA", "ORAA", "ADDA", "CPX", "JSR", "LDS", "STS",
	};
	static const char *bops[16] = {
		"SUBB", "CMPB", "SBCB", "ADDD", "ANDB", "BITB", "LDAB", "STAB",
		"EORB", "ADCB", "ORAB", "ADDB", "LDD", "STD", "LDX", "STX",
	};
	static const enum m6801_mode modes[4] = { M_IMM, M_DIR, M_IDX, M_EXT };
	int i, m;

	for (i = 0; i < 0x40; i++)
		if (inh[i])
			setop(i, inh[i], M_INH);
	for (i = 0; i < 16; i++) {
		setop(0x20 + i, branch[i], M_REL);
		if (rmwa[i])
			setop(0x40 + i, rmwa[i], M_INH);
		if (rmwb[i])
			setop(0x50 + i, rmwb[i], M_INH);
		if (rmw[i]) {
			setop(0x60 + i, rmw[i], M_IDX);
			setop(0x70 + i, rmw[i], M_EXT);
		}
		for (m = 0; m < 4; m++) {
			enum m6801_mode mode = modes[m];

			/* 16-bit immediate operands */
			if (mode == M_IMM && (i == 0x3 || i == 0xC || i == 0xE))
				mode = M_IMM16;
			setop(0x80 + 0x10 * m + i, aops[i], mode);
			setop(0xC0 + 0x10 * m + i, bops[i], mode);
		}
	}
	/* immediate stores don't exist; $8D is BSR, not JSR */
	m6801_ops[0x87].mode = m6801_ops[0x8F].mode = M_ILL;
	m6801_ops[0xC7].mode = m6801_ops[0xCD].mode =
	    m6801_ops[0xCF].mode = M_ILL;
	setop(0x8D, "BSR", M_REL);
}

unsigned m6801_oplength(enum m6801_mode mode)
{
	switch (mode) {
	case M_INH:
		return 1;
	case M_IMM16:
	case M_EXT:
		return 3;
	default:
		return 2;
	}
}
