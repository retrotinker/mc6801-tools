/* m6801ops.h - the MC6801 instruction set: each opcode's mnemonic and
   addressing mode, for objdump01's disassembler and the tests' simulator */

enum m6801_mode {
	M_ILL,			/* not an instruction */
	M_INH,			/* inherent: no operand */
	M_IMM,			/* 8-bit immediate */
	M_IMM16,		/* 16-bit immediate */
	M_DIR,			/* direct: 8-bit address in page zero */
	M_IDX,			/* indexed: 8-bit unsigned offset from X */
	M_EXT,			/* extended: 16-bit address */
	M_REL			/* relative: 8-bit signed branch offset */
};

struct m6801_op {
	const char *name;	/* mnemonic, in capitals */
	enum m6801_mode mode;
};

extern struct m6801_op m6801_ops[256];

void m6801_init_ops(void);	/* fill in m6801_ops */
unsigned m6801_oplength(enum m6801_mode mode);	/* bytes, with the opcode */
