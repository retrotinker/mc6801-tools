/* mops.c - handle pseudo-ops */

#include "syshead.h"
#include "const.h"
#include "type.h"
#include "globvar.h"
#include "opcode.h"
#include "scan.h"
#undef EXTERN
#define EXTERN
#include "address.h"

#define is8bitadr(offset) ((offset_t) offset < 0x100)
#define is8bitsignedoffset(offset) ((offset_t) (offset) + 0x80 < 0x100)
#define pass2 (pass==last_pass)

FORWARD void mshort2 P((void));
FORWARD reg_pt regchk P((void));
FORWARD void reldata P((void));
FORWARD void segadj P((void));

/* 6801/6803 opcode constants */

/* bits for indexed addressing */

#define INDIRECTBIT 0x10
#define INDEXBIT    0x80	/* except 5 bit offset */
#define RRBITS      0x60	/* register select bits */

PRIVATE opcode_t rrindex[] =	/* register and index bits for indexed adr */
{
	0x00,		/* X */
};

FORWARD void doaltind P((void));
FORWARD void do1altind P((void));
FORWARD void fixupind P((void));
FORWARD void getindexnopost P((void));
FORWARD void inderror P((char *err_str));
FORWARD reg_pt indreg P((reg_pt maxindex));
FORWARD void predec1 P((void));

/* common code for all-mode ops, alterable-mode ops, indexed ops */

PRIVATE void doaltind()
{
	bool_t byteflag;	/* set if direct or short indexed adr forced */
	char *oldlineptr;
	char *oldsymname;
	reg_pt reg;
	bool_t wordflag;	/* set if extended or long indexed adr forced */

	mcount += 0x2;

	if (sym == SUBOP) {	/* could be -R or - in expression */
		oldlineptr = lineptr;	/* save state */
		oldsymname = symname;
		getsym();
		reg = regchk();
		lineptr = oldlineptr;
		symname = oldsymname;
		if (reg != NOREG) {
			predec1();	/* it's -R */
			return;
		}
		sym = SUBOP;
	} else if (sym == COMMA) {
		lastexp.offset = 0;	/* hack to simulate "0,x" */
		getsym();
		if (indreg(MAXINDREG) != NOREG) {
			fixupind();
			return;
		}
	}
	if (sym == PREDECOP) {
		postb |= 0x83;
		getindexnopost();
		return;
	}

	/* should have expression */

	wordflag = byteflag = FALSE;
	if (sym == LESSTHAN) {
		/* context-sensitive, LESSTHAN means byte-sized here */
		byteflag = TRUE;
		getsym();
	} else if (sym == GREATERTHAN) {
		/* context-sensitive, GREATERTHAN means word-sized here */
		wordflag = TRUE;
		getsym();
	}
	expres();
	if (sym == COMMA) {	/* offset from register */
		getsym();
		if ((reg = indreg(AREG)) == NOREG)
			return;
		if (wordflag || !is8bitadr(lastexp.offset))
			inderror(ILLMOD);	/* 8 bit unsigned offset only */
		fixupind();
	} else if (postb & INDIRECTBIT) {	/* extended indirect */
		postb = 0x9F;
		mcount += 0x2;
		fixupind();
	} else if (postb & INDEXBIT)
		inderror(ILLMOD);	/* e.g. LEAX     $10 */
	else {
		if (byteflag || (!wordflag && !(lastexp.data & (FORBIT | RELBIT)) && (lastexp.offset >> 0x8) == dirpag)) {	/* direct addressing */
			if (opcode >= 0x80)
				opcode |= 0x10;
		} else {	/* extended addressing */

			if (opcode < 0x80)
				opcode |= 0x70;
			else
				opcode |= 0x30;
			++mcount;
			if (pass2
			    && (opcode == JSR_OPCODE || opcode == JMP_OPCODE)
			    && !(lastexp.data & IMPBIT)
			    && lastexp.offset + (0x81 - 0x3) < 0x101)
				/* JSR or JMP could be done with BSR or BRA */
				warning(SHORTB);
		}
	}
}

PRIVATE void fixupind()
{
	if ((opcode & 0x30) == 0x0) {	/* change all but LEA opcodes */
		if (opcode < 0x80)
			opcode |= 0x60;
		else
			opcode |= 0x20;
	}
}

PRIVATE void getindexnopost()
{
	getsym();
	if (indreg(MAXINDREG) != NOREG)
		fixupind();
}

PRIVATE void inderror(err_str)
char *err_str;
{
	error(err_str);
	if (postb & INDIRECTBIT)
		sym = RBRACKET;	/* fake right bracket to kill further errors */
	fixupind();
}

/* check current symbol is an index register (possibly excepting PC) */
/* if so, modify postbyte RR and INDEXBIT for it, get next sym, return TRUE */
/* otherwise generate error, return FALSE */

PRIVATE reg_pt indreg(maxindex)
reg_pt maxindex;
{
	reg_pt reg;

	if ((reg = regchk()) == NOREG)
		inderror(IREGEXP);
	else if (reg > maxindex) {
		inderror(ILLREG);
		reg = NOREG;
	} else {
		postb |= rrindex[reg];
		getsym();
	}
	return reg;
}

/* all-mode ops */

PUBLIC void mall()
{
	if (sym == IMMEDIATE)
		mimmed();
	else
		malter();
}

/* alterable mode ops */

PUBLIC void malter()
{
	postb = 0x0;		/* not yet indexed or indirect */
	doaltind();
}

/* immediate ops */

PUBLIC void mimmed()
{
	opcode_t nybble;

	mcount += 0x2;
	if (sym != IMMEDIATE)
		error(ILLMOD);
	else {
		if (opcode >= 0x80 && ((nybble = opcode & 0xF) == 0x3 ||
				       nybble == 0xC || nybble >= 0xE))
			++mcount;	/* magic for long immediate */
		symexpres();
		if (pass2 && mcount <= 0x2) {
			chkabs();
			checkdatabounds();
		}
	}
}

PRIVATE void predec1()
{
	if (postb & INDIRECTBIT)
		inderror(ILLMOD);	/* single-dec indirect illegal */
	else {
		postb |= 0x82;
		getindexnopost();
	}
}

/* routines common to all processors */

PUBLIC void getcomma()
{
	if (sym != COMMA)
		error(COMEXP);
	else
		getsym();
}

/* inherent ops */

/* for I80386 */
/* AAA, AAS, CLC, CLD, CLI, CLTS, CMC, CMPSB, DAA, DAS, HLT, INTO, INSB, */
/* INVD, */
/* LAHF, LEAVE, LOCK, LODSB, MOVSB, NOP, OUTSB, REP, REPE, REPNE, REPNZ, */
/* REPZ, SAHF, SCASB, STC, STD, STI, STOSB, WAIT, WBINVD */

PUBLIC void minher()
{
	++mcount;
}

/* short branches */

PUBLIC void mshort()
{
	nonimpexpres();
	mshort2();
}

PRIVATE void mshort2()
{
	mcount += 0x2;
	if (pass2) {
		reldata();
		if (lastexp.data & RELBIT)
			showrelbad();
		else if (!(lastexp.data & UNDBIT)) {
			lastexp.offset = lastexp.offset - lc - mcount;
			if (!is8bitsignedoffset(lastexp.offset))
				error(ABOUNDS);
		}
	}
}

/* check if current symbol is a register, return register number or NOREG */

PRIVATE reg_pt regchk()
{
	register struct sym_s *symptr;

	if (sym == IDENT) {
		if ((symptr = gsymptr)->type & MNREGBIT) {
			if (symptr->data & REGBIT) {
				int regno = symptr->value_reg_or_op.reg;
				return regno;
			}
		} else if (last_pass == 1)
			if (!(symptr->type & (LABIT | MACBIT | VARBIT)))
				symptr->data |= FORBIT;	/* show seen in advance */
	}
	return NOREG;
}

/* convert lastexp.data for PC relative */

PRIVATE void reldata()
{
	if ((lastexp.data ^ lcdata) & (IMPBIT | RELBIT | SEGM)) {
		if ((lastexp.data ^ lcdata) & RELBIT)
			showrelbad();	/* rel - abs is weird, abs - rel is bad */
		else {
			pcrflag = OBJ_R_MASK;
			lastexp.data = (lcdata & ~SEGM) | lastexp.data | RELBIT;
			/* segment is that of lastexp.data */
		}
	} else			/* same file, segment and relocation */
		lastexp.data = (lastexp.data | lcdata) & ~(RELBIT | SEGM);
}

PRIVATE void segadj()
{
	if ((lastexp.data & UNDBIT) && textseg >= 0) {
		lastexp.sym->data &= ~SEGM;
		lastexp.sym->data |= (lcdata & SEGM);
	}
}
