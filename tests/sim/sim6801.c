/*
 * sim6801 - an MC6801 instruction set simulator for the regression tests
 *
 * Loads a program (Motorola S-records, or a raw binary at -a), runs it
 * from its entry point until it stops, and prints the registers and any
 * memory ranges asked for with -d.  The tests compare that output with
 * references, so the programs the tools build are run, not just compared
 * byte by byte.
 *
 * Memory is a flat 64 KB of RAM, initially zero.  The MC6801's internal
 * registers ($00-$1F) are ordinary memory, and there are no interrupts
 * apart from SWI.
 *
 * usage: sim6801 [options] file
 *   -a addr       load a raw binary file at addr (default: S-records)
 *   -e addr       start at addr (default: the S9 record's address, or
 *                 the reset vector at $FFFE)
 *   -s addr       initial stack pointer (default $00FF)
 *   -r addr       call the program as a subroutine returning to addr:
 *                 push addr as the return address, and stop there
 *   -x addr       stop when execution reaches addr (repeatable)
 *   -p addr       character output: when execution reaches addr, write
 *                 the character in A (CR as a newline) and return, like
 *                 the MC-10 ROM's output routine (repeatable)
 *   -w addr=word  store a 16-bit word before running (repeatable)
 *   -n count      stop after count instructions (otherwise running for
 *                 10 million instructions is an error)
 *   -d from-to    after the run, dump memory from from to to (repeatable)
 *   -t            trace each instruction on standard error
 *   -L            list: decode the loaded bytes as instructions (address,
 *                 opcode and mnemonic) instead of running them
 *
 * Numbers are C-style: 0x1234, 4660 or 011064.  Execution stops normally
 * (exit status 0) at a -x or -r address, at a branch or jump to itself
 * (the usual "halt" loop), at WAI, or after -n instructions; an illegal
 * opcode or a runaway program is an error (exit status 1).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXADDRS	16
#define RUNAWAY		10000000UL

/* condition code bits; bits 6 and 7 always read as 1 */
#define CC_H	0x20
#define CC_I	0x10
#define CC_N	0x08
#define CC_Z	0x04
#define CC_V	0x02
#define CC_C	0x01

enum mode { ILL, INH, IMM, IMM16, DIR, IDX, EXT, REL };

struct opinfo {
	const char *name;
	enum mode mode;
};

static struct opinfo ops[256];

static unsigned char mem[0x10000];
static int loaded[0x10000];		/* for -L: which bytes were loaded */
static unsigned a, b, x, sp, pc, cc;

static const char *progname = "sim6801";

static void fatal(const char *msg, const char *arg)
{
	fprintf(stderr, "%s: %s%s%s\n", progname, msg, arg ? ": " : "",
		arg ? arg : "");
	exit(1);
}

/* the instruction table */

static void setop(int code, const char *name, enum mode mode)
{
	ops[code].name = name;
	ops[code].mode = mode;
}

static void init_ops(void)
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
	static const enum mode modes[4] = { IMM, DIR, IDX, EXT };
	int i, m;

	for (i = 0; i < 0x40; i++)
		if (inh[i])
			setop(i, inh[i], INH);
	for (i = 0; i < 16; i++) {
		setop(0x20 + i, branch[i], REL);
		if (rmwa[i])
			setop(0x40 + i, rmwa[i], INH);
		if (rmwb[i])
			setop(0x50 + i, rmwb[i], INH);
		if (rmw[i]) {
			setop(0x60 + i, rmw[i], IDX);
			setop(0x70 + i, rmw[i], EXT);
		}
		for (m = 0; m < 4; m++) {
			enum mode mode = modes[m];

			/* 16-bit immediate operands */
			if (mode == IMM && (i == 0x3 || i == 0xC || i == 0xE))
				mode = IMM16;
			setop(0x80 + 0x10 * m + i, aops[i], mode);
			setop(0xC0 + 0x10 * m + i, bops[i], mode);
		}
	}
	/* immediate stores don't exist; $8D is BSR, not JSR */
	ops[0x87].mode = ops[0x8F].mode = ILL;
	ops[0xC7].mode = ops[0xCD].mode = ops[0xCF].mode = ILL;
	setop(0x8D, "BSR", REL);
}

static unsigned oplength(enum mode mode)
{
	switch (mode) {
	case INH:
		return 1;
	case IMM16:
	case EXT:
		return 3;
	default:
		return 2;
	}
}

/* memory and the stack */

static unsigned read16(unsigned addr)
{
	return (mem[addr & 0xFFFF] << 8) | mem[(addr + 1) & 0xFFFF];
}

static void write16(unsigned addr, unsigned value)
{
	mem[addr & 0xFFFF] = value >> 8;
	mem[(addr + 1) & 0xFFFF] = value & 0xFF;
}

static void push8(unsigned value)
{
	mem[sp] = value;
	sp = (sp - 1) & 0xFFFF;
}

static unsigned pull8(void)
{
	sp = (sp + 1) & 0xFFFF;
	return mem[sp];
}

/* 16-bit values go on the stack low byte first, so they read in order */
static void push16(unsigned value)
{
	push8(value & 0xFF);
	push8(value >> 8);
}

static unsigned pull16(void)
{
	unsigned hi = pull8();

	return (hi << 8) | pull8();
}

/* condition codes */

static void flag(unsigned bit, int set)
{
	if (set)
		cc |= bit;
	else
		cc &= ~bit;
}

static void nz8(unsigned r)
{
	flag(CC_N, r & 0x80);
	flag(CC_Z, (r & 0xFF) == 0);
}

static void nz16(unsigned r)
{
	flag(CC_N, r & 0x8000);
	flag(CC_Z, (r & 0xFFFF) == 0);
}

/* N, Z and V = 0, for loads, stores and logical operations */
static unsigned logic8(unsigned r)
{
	nz8(r);
	flag(CC_V, 0);
	return r & 0xFF;
}

static unsigned logic16(unsigned r)
{
	nz16(r);
	flag(CC_V, 0);
	return r & 0xFFFF;
}

/* V = N exclusive-or C, after a shift or rotate */
static void shiftv(void)
{
	flag(CC_V, !(cc & CC_N) != !(cc & CC_C));
}

static unsigned add8(unsigned m, unsigned n, unsigned carry)
{
	unsigned r = m + n + carry;

	flag(CC_H, (m ^ n ^ r) & 0x10);
	flag(CC_V, ~(m ^ n) & (m ^ r) & 0x80);
	flag(CC_C, r & 0x100);
	nz8(r);
	return r & 0xFF;
}

static unsigned sub8(unsigned m, unsigned n, unsigned borrow)
{
	unsigned r = m - n - borrow;

	flag(CC_V, (m ^ n) & (m ^ r) & 0x80);
	flag(CC_C, r & 0x100);
	nz8(r);
	return r & 0xFF;
}

static unsigned add16(unsigned m, unsigned n)
{
	unsigned r = m + n;

	flag(CC_V, ~(m ^ n) & (m ^ r) & 0x8000);
	flag(CC_C, r & 0x10000);
	nz16(r);
	return r & 0xFFFF;
}

static unsigned sub16(unsigned m, unsigned n)
{
	unsigned r = m - n;

	flag(CC_V, (m ^ n) & (m ^ r) & 0x8000);
	flag(CC_C, r & 0x10000);
	nz16(r);
	return r & 0xFFFF;
}

/* NEG..CLR ($40-$7F) on the value m; returns the result */
static unsigned rmw(unsigned op, unsigned m)
{
	unsigned r = m;

	switch (op) {
	case 0x0:		/* NEG */
		r = (0 - m) & 0xFF;
		flag(CC_V, r == 0x80);
		flag(CC_C, r != 0);
		nz8(r);
		break;
	case 0x3:		/* COM */
		r = logic8(~m);
		flag(CC_C, 1);
		break;
	case 0x4:		/* LSR */
		flag(CC_C, m & 1);
		r = m >> 1;
		nz8(r);
		shiftv();
		break;
	case 0x6:		/* ROR */
		r = (m >> 1) | (cc & CC_C ? 0x80 : 0);
		flag(CC_C, m & 1);
		nz8(r);
		shiftv();
		break;
	case 0x7:		/* ASR */
		r = (m >> 1) | (m & 0x80);
		flag(CC_C, m & 1);
		nz8(r);
		shiftv();
		break;
	case 0x8:		/* ASL */
		r = (m << 1) & 0xFF;
		flag(CC_C, m & 0x80);
		nz8(r);
		shiftv();
		break;
	case 0x9:		/* ROL */
		r = ((m << 1) | (cc & CC_C)) & 0xFF;
		flag(CC_C, m & 0x80);
		nz8(r);
		shiftv();
		break;
	case 0xA:		/* DEC */
		r = (m - 1) & 0xFF;
		flag(CC_V, m == 0x80);
		nz8(r);
		break;
	case 0xC:		/* INC */
		r = (m + 1) & 0xFF;
		flag(CC_V, m == 0x7F);
		nz8(r);
		break;
	case 0xD:		/* TST */
		logic8(m);
		flag(CC_C, 0);
		break;
	case 0xF:		/* CLR */
		r = logic8(0);
		flag(CC_C, 0);
		break;
	}
	return r;
}

static int branch_taken(unsigned op)
{
	int n = !!(cc & CC_N), z = !!(cc & CC_Z);
	int v = !!(cc & CC_V), c = !!(cc & CC_C);
	int taken;

	switch (op >> 1) {
	case 0:			/* BRA, BRN */
		taken = 1;
		break;
	case 1:			/* BHI, BLS */
		taken = !(c | z);
		break;
	case 2:			/* BCC, BCS */
		taken = !c;
		break;
	case 3:			/* BNE, BEQ */
		taken = !z;
		break;
	case 4:			/* BVC, BVS */
		taken = !v;
		break;
	case 5:			/* BPL, BMI */
		taken = !n;
		break;
	case 6:			/* BGE, BLT */
		taken = !(n ^ v);
		break;
	default:		/* BGT, BLE */
		taken = !(z | (n ^ v));
		break;
	}
	return op & 1 ? !taken : taken;
}

/* running */

enum stop { RUNNING, AT_ADDRESS, SELF_LOOP, WAITING, COUNT, ILLEGAL };

static unsigned stops[MAXADDRS], nstops;
static unsigned outputs[MAXADDRS], noutputs;
static int trace;

static int listed(const unsigned *list, unsigned n, unsigned addr)
{
	unsigned i;

	for (i = 0; i < n; i++)
		if (list[i] == addr)
			return 1;
	return 0;
}

static void putout(unsigned ch)
{
	putchar(ch == 0x0D ? '\n' : (int)ch);
}

/* execute one instruction */
static enum stop step(void)
{
	unsigned start = pc, op, lo, ea = 0, m = 0;
	enum mode mode;

	if (listed(stops, nstops, pc))
		return AT_ADDRESS;
	if (listed(outputs, noutputs, pc)) {
		putout(a);
		pc = pull16();
		return RUNNING;
	}

	op = mem[pc];
	mode = ops[op].mode;
	if (trace)
		fprintf(stderr, "%04X  %02X %-5s A=%02X B=%02X X=%04X "
			"SP=%04X CC=%02X\n", pc, op,
			mode == ILL ? "???" : ops[op].name, a, b, x, sp, cc);
	if (mode == ILL)
		return ILLEGAL;
	pc = (pc + 1) & 0xFFFF;

	switch (mode) {
	case IMM:
		ea = pc;
		pc = (pc + 1) & 0xFFFF;
		break;
	case IMM16:
		ea = pc;
		pc = (pc + 2) & 0xFFFF;
		break;
	case DIR:
		ea = mem[pc];
		pc = (pc + 1) & 0xFFFF;
		break;
	case IDX:
		ea = (x + mem[pc]) & 0xFFFF;
		pc = (pc + 1) & 0xFFFF;
		break;
	case EXT:
		ea = read16(pc);
		pc = (pc + 2) & 0xFFFF;
		break;
	case REL:
		ea = (pc + 1 + (signed char)mem[pc]) & 0xFFFF;
		pc = (pc + 1) & 0xFFFF;
		break;
	default:
		break;
	}

	lo = op & 0x0F;
	if (op < 0x20) {
		unsigned d;

		switch (op) {
		case 0x01:	/* NOP */
			break;
		case 0x04:	/* LSRD */
			d = (a << 8) | b;
			flag(CC_C, d & 1);
			d >>= 1;
			nz16(d);
			shiftv();
			a = d >> 8;
			b = d & 0xFF;
			break;
		case 0x05:	/* ASLD */
			d = (a << 8) | b;
			flag(CC_C, d & 0x8000);
			d = (d << 1) & 0xFFFF;
			nz16(d);
			shiftv();
			a = d >> 8;
			b = d & 0xFF;
			break;
		case 0x06:	/* TAP */
			cc = a | 0xC0;
			break;
		case 0x07:	/* TPA */
			a = cc;
			break;
		case 0x08:	/* INX */
			x = (x + 1) & 0xFFFF;
			flag(CC_Z, x == 0);
			break;
		case 0x09:	/* DEX */
			x = (x - 1) & 0xFFFF;
			flag(CC_Z, x == 0);
			break;
		case 0x0A:	/* CLV */
		case 0x0B:	/* SEV */
			flag(CC_V, op & 1);
			break;
		case 0x0C:	/* CLC */
		case 0x0D:	/* SEC */
			flag(CC_C, op & 1);
			break;
		case 0x0E:	/* CLI */
		case 0x0F:	/* SEI */
			flag(CC_I, op & 1);
			break;
		case 0x10:	/* SBA */
			a = sub8(a, b, 0);
			break;
		case 0x11:	/* CBA */
			sub8(a, b, 0);
			break;
		case 0x16:	/* TAB */
			b = logic8(a);
			break;
		case 0x17:	/* TBA */
			a = logic8(b);
			break;
		case 0x19:	/* DAA */
			{
				unsigned msn = a & 0xF0, lsn = a & 0x0F;
				unsigned fix = 0;

				if ((cc & CC_H) || lsn > 9)
					fix |= 0x06;
				if ((cc & CC_C) || msn > 0x90
				    || (msn > 0x80 && lsn > 9))
					fix |= 0x60;
				a = logic8(a + fix);
				if (fix & 0x60)
					flag(CC_C, 1);
			}
			break;
		case 0x1B:	/* ABA */
			a = add8(a, b, 0);
			break;
		}
	} else if (op < 0x30) {
		if (branch_taken(lo)) {
			if (op == 0x20 && ea == start) {
				pc = start;
				return SELF_LOOP;
			}
			pc = ea;
		}
	} else if (op < 0x40) {
		switch (op) {
		case 0x30:	/* TSX */
			x = (sp + 1) & 0xFFFF;
			break;
		case 0x31:	/* INS */
			sp = (sp + 1) & 0xFFFF;
			break;
		case 0x32:	/* PULA */
			a = pull8();
			break;
		case 0x33:	/* PULB */
			b = pull8();
			break;
		case 0x34:	/* DES */
			sp = (sp - 1) & 0xFFFF;
			break;
		case 0x35:	/* TXS */
			sp = (x - 1) & 0xFFFF;
			break;
		case 0x36:	/* PSHA */
			push8(a);
			break;
		case 0x37:	/* PSHB */
			push8(b);
			break;
		case 0x38:	/* PULX */
			x = pull16();
			break;
		case 0x39:	/* RTS */
			pc = pull16();
			break;
		case 0x3A:	/* ABX */
			x = (x + b) & 0xFFFF;
			break;
		case 0x3B:	/* RTI */
			cc = pull8() | 0xC0;
			b = pull8();
			a = pull8();
			x = pull16();
			pc = pull16();
			break;
		case 0x3C:	/* PSHX */
			push16(x);
			break;
		case 0x3D:	/* MUL */
			{
				unsigned d = a * b;

				a = d >> 8;
				b = d & 0xFF;
				flag(CC_C, b & 0x80);
			}
			break;
		case 0x3E:	/* WAI */
			return WAITING;
		case 0x3F:	/* SWI */
			push16(pc);
			push16(x);
			push8(a);
			push8(b);
			push8(cc);
			flag(CC_I, 1);
			pc = read16(0xFFFA);
			break;
		}
	} else if (op < 0x50) {
		a = rmw(lo, a);
	} else if (op < 0x60) {
		b = rmw(lo, b);
	} else if (op < 0x80) {
		if (lo == 0xE) {	/* JMP */
			if (ea == start) {
				pc = start;
				return SELF_LOOP;
			}
			pc = ea;
		} else {
			m = rmw(lo, mem[ea]);
			if (lo != 0xD)	/* TST doesn't write */
				mem[ea] = m;
		}
	} else {
		int isb = op >= 0xC0;
		unsigned *r = isb ? &b : &a;
		unsigned d = (a << 8) | b;

		switch (lo) {
		case 0x0:	/* SUB */
			*r = sub8(*r, mem[ea], 0);
			break;
		case 0x1:	/* CMP */
			sub8(*r, mem[ea], 0);
			break;
		case 0x2:	/* SBC */
			*r = sub8(*r, mem[ea], cc & CC_C);
			break;
		case 0x3:	/* SUBD, ADDD */
			d = isb ? add16(d, read16(ea)) : sub16(d, read16(ea));
			a = d >> 8;
			b = d & 0xFF;
			break;
		case 0x4:	/* AND */
			*r = logic8(*r & mem[ea]);
			break;
		case 0x5:	/* BIT */
			logic8(*r & mem[ea]);
			break;
		case 0x6:	/* LDA */
			*r = logic8(mem[ea]);
			break;
		case 0x7:	/* STA */
			mem[ea] = logic8(*r);
			break;
		case 0x8:	/* EOR */
			*r = logic8(*r ^ mem[ea]);
			break;
		case 0x9:	/* ADC */
			*r = add8(*r, mem[ea], cc & CC_C);
			break;
		case 0xA:	/* ORA */
			*r = logic8(*r | mem[ea]);
			break;
		case 0xB:	/* ADD */
			*r = add8(*r, mem[ea], 0);
			break;
		case 0xC:	/* CPX, LDD */
			if (isb) {
				d = logic16(read16(ea));
				a = d >> 8;
				b = d & 0xFF;
			} else
				sub16(x, read16(ea));
			break;
		case 0xD:	/* BSR, JSR, STD */
			if (isb)
				write16(ea, logic16(d));
			else {
				push16(pc);
				pc = ea;
			}
			break;
		case 0xE:	/* LDS, LDX */
			if (isb)
				x = logic16(read16(ea));
			else
				sp = logic16(read16(ea));
			break;
		case 0xF:	/* STS, STX */
			write16(ea, logic16(isb ? x : sp));
			break;
		}
	}
	return RUNNING;
}

/* loading */

static int hexval(const char *s, int digits, unsigned *value)
{
	unsigned v = 0;
	int i;

	for (i = 0; i < digits; i++) {
		int c = s[i];

		v <<= 4;
		if (c >= '0' && c <= '9')
			v |= c - '0';
		else if (c >= 'A' && c <= 'F')
			v |= c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			v |= c - 'a' + 10;
		else
			return 0;
	}
	*value = v;
	return 1;
}

/* load S-records; returns 1 and sets *entry if there was an S9 record */
static int load_srec(FILE *f, const char *name, unsigned *entry)
{
	char line[600];
	int have_entry = 0;

	while (fgets(line, sizeof line, f)) {
		unsigned count, sum, v, addr = 0, i;
		size_t len = strcspn(line, "\r\n");

		line[len] = 0;
		if (len == 0)
			continue;
		if (line[0] != 'S' || len < 4 || !hexval(line + 2, 2, &count)
		    || len != 4 + 2 * (size_t)count)
			fatal("bad S-record", name);
		sum = count;
		for (i = 0; i < count; i++) {
			if (!hexval(line + 4 + 2 * i, 2, &v))
				fatal("bad S-record", name);
			sum += v;
		}
		if ((sum & 0xFF) != 0xFF)
			fatal("S-record checksum error", name);
		switch (line[1]) {
		case '1':
			hexval(line + 4, 4, &addr);
			for (i = 0; i + 3 < count; i++) {
				hexval(line + 8 + 2 * i, 2, &v);
				mem[(addr + i) & 0xFFFF] = v;
				loaded[(addr + i) & 0xFFFF] = 1;
			}
			break;
		case '9':
			hexval(line + 4, 4, entry);
			have_entry = 1;
			break;
		case '0':
		case '5':
			break;
		default:
			fatal("unsupported S-record type", name);
		}
	}
	return have_entry;
}

static void load_binary(FILE *f, unsigned addr)
{
	int c;

	while ((c = getc(f)) != EOF) {
		if (addr > 0xFFFF)
			fatal("binary file doesn't fit in memory", 0);
		mem[addr] = c;
		loaded[addr++] = 1;
	}
}

/* -L: decode the loaded bytes, in address order */
static void list(void)
{
	unsigned addr = 0;

	while (addr <= 0xFFFF) {
		struct opinfo *o;

		if (!loaded[addr]) {
			addr++;
			continue;
		}
		o = &ops[mem[addr]];
		printf("%04X  %02X  %s\n", addr, mem[addr],
		       o->mode == ILL ? "???" : o->name);
		addr += o->mode == ILL ? 1 : oplength(o->mode);
	}
}

/* the command line */

static unsigned number(const char *s, unsigned max)
{
	char *end;
	unsigned long v = strtoul(s, &end, 0);

	if (*s == 0 || *end != 0 || v > max)
		fatal("bad number", s);
	return v;
}

static void usage(void)
{
	fprintf(stderr,
		"usage: %s [-a addr] [-e addr] [-s addr] [-r addr] [-x addr]"
		" [-p addr]\n"
		"       [-w addr=word] [-n count] [-d from-to] [-t] [-L]"
		" file\n", progname);
	exit(1);
}

int main(int argc, char **argv)
{
	static const char *reasons[] = {
		"", "stopped", "branch to itself", "WAI",
		"instruction count reached", "illegal opcode",
	};
	unsigned dumps[MAXADDRS][2], ndumps = 0;
	unsigned long count = 0, limit = RUNAWAY;
	int have_limit = 0, have_entry = 0, raw = 0, listing = 0;
	unsigned loadaddr = 0, entry = 0, i;
	int have_return = 0, have_start = 0;
	unsigned retaddr = 0, start = 0;
	const char *file;
	enum stop why;
	FILE *f;
	int argn;

	init_ops();
	sp = 0x00FF;
	cc = 0xC0 | CC_I;

	for (argn = 1; argn < argc && argv[argn][0] == '-'; argn++) {
		const char *opt = argv[argn];
		const char *arg;

		if (strcmp(opt, "-t") == 0) {
			trace = 1;
			continue;
		}
		if (strcmp(opt, "-L") == 0) {
			listing = 1;
			continue;
		}
		if (strlen(opt) != 2 || ++argn >= argc)
			usage();
		arg = argv[argn];
		switch (opt[1]) {
		case 'a':
			loadaddr = number(arg, 0xFFFF);
			raw = 1;
			break;
		case 'e':
			start = number(arg, 0xFFFF);
			have_start = 1;
			break;
		case 's':
			sp = number(arg, 0xFFFF);
			break;
		case 'r':
			retaddr = number(arg, 0xFFFF);
			have_return = 1;
			/* FALLTHROUGH */
		case 'x':
			if (nstops == MAXADDRS)
				fatal("too many -x addresses", 0);
			stops[nstops++] = number(arg, 0xFFFF);
			break;
		case 'p':
			if (noutputs == MAXADDRS)
				fatal("too many -p addresses", 0);
			outputs[noutputs++] = number(arg, 0xFFFF);
			break;
		case 'w':
			{
				char buf[64], *eq;

				if (strlen(arg) >= sizeof buf
				    || !(eq = strchr(strcpy(buf, arg), '=')))
					fatal("bad -w argument", arg);
				*eq = 0;
				write16(number(buf, 0xFFFF),
					number(eq + 1, 0xFFFF));
			}
			break;
		case 'n':
			limit = number(arg, 0xFFFFFFFFU);
			have_limit = 1;
			break;
		case 'd':
			{
				char buf[64], *dash;

				if (ndumps == MAXADDRS)
					fatal("too many -d ranges", 0);
				if (strlen(arg) >= sizeof buf
				    || !(dash = strchr(strcpy(buf, arg), '-')))
					fatal("bad -d argument", arg);
				*dash = 0;
				dumps[ndumps][0] = number(buf, 0xFFFF);
				dumps[ndumps][1] = number(dash + 1, 0xFFFF);
				if (dumps[ndumps][1] < dumps[ndumps][0])
					fatal("bad -d argument", arg);
				ndumps++;
			}
			break;
		default:
			usage();
		}
	}
	if (argn != argc - 1)
		usage();
	file = argv[argn];

	if (!(f = fopen(file, raw ? "rb" : "r")))
		fatal("cannot open", file);
	if (raw)
		load_binary(f, loadaddr);
	else
		have_entry = load_srec(f, file, &entry);
	fclose(f);

	if (listing) {
		list();
		return 0;
	}

	pc = have_start ? start : have_entry ? entry : read16(0xFFFE);
	if (have_return)
		push16(retaddr);

	while ((why = step()) == RUNNING)
		if (++count == limit) {
			why = COUNT;
			break;
		}

	fflush(stdout);
	if (why == COUNT && !have_limit) {
		fprintf(stderr, "%s: no stop after %lu instructions\n",
			progname, count);
		return 1;
	}
	printf("%s at %04X after %lu instructions\n", reasons[why], pc,
	       count);
	printf("A=%02X B=%02X X=%04X SP=%04X CC=%02X\n", a, b, x, sp, cc);
	for (i = 0; i < ndumps; i++) {
		unsigned addr;

		for (addr = dumps[i][0]; addr <= dumps[i][1]; addr++) {
			if (addr == dumps[i][0] || addr % 16 == 0)
				printf("%s%04X:", addr == dumps[i][0] ? ""
				       : "\n", addr);
			printf(" %02X", mem[addr]);
		}
		printf("\n");
	}
	if (why == ILLEGAL) {
		fflush(stdout);
		fprintf(stderr, "%s: illegal opcode %02X at %04X\n",
			progname, mem[pc], pc);
		return 1;
	}
	return 0;
}
