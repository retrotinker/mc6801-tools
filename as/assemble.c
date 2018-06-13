/* assemble.c - main loop for assembler */

#include "syshead.h"
#include "const.h"
#include "type.h"
#include "address.h"
#include "globvar.h"
#include "opcode.h"
#include "scan.h"

PRIVATE bool_t nocolonlabel;	/* set for labels not followed by ':' */
PRIVATE void (*routine) P((void));
PRIVATE pfv rout_table[] = {
	pelse,
	pelseif,
	pelsifc,
	pendif,
	pif,
	pifc,

	/* start of non-conditionals */
	palign,
	pasciz,
	pblkw,
	pblock,
	pbss,
	pcomm,
	pcomm1,
	pdata,
	pendb,
	penter,
	pentry,
	pequ,
	peven,
	pexport,
	pfail,
	pfcb,
	pfcc,
	pfdb,
#if SIZEOF_OFFSET_T > 2
	pfqb,
#endif
	pget,
	pglobl,
	pident,
	pimport,
	plcomm,
	plcomm1,
	plist,
	ploc,
	pmaclist,
	pmacro,
	pmap,
	porg,
	pproceof,
	prmb,
	psect,
	pset,
	ptext,
	pwarn,
	/* end of pseudo-ops */

	mall,			/* all address modes allowed, like LDA */
	malter,			/* all but immediate, like STA */
	mimmed,			/* immediate only (ANDCC, ORCC) */
	minher,			/* inherent, like CLC or CLRA */
	mshort,			/* short branches */
};

FORWARD void asline P((void));

/*
  This uses registers as follows: A is for work and is not preserved by
  the subroutines.B holds the last symbol code, X usually points to data
  about the last symbol, U usually holds the value of last expression
  or symbol, and Y points to the current char. The value in Y is needed
  by READCH and GETSYM.  EXPRES needs B and Y, and returns a value in U.
  If the expression starts with an identifier, X must point to its string.
  LOOKUP needs a string pointer in X and length in A. It returns a table
  pointer in X (unless not assembling and not found), symbol type in A
  and overflow in CC.
*/

PUBLIC void assemble()
{
	while (TRUE) {
		asline();
		if (label != NUL_PTR) {	/* must be confirmed if still set *//* it is nulled by EQU, COMM and SET */
			if (pass && label->value_reg_or_op.value != oldlabel) {
				dirty_pass = TRUE;
				if (pass == last_pass)
					error(UNSTABLE_LABEL);
			}

			label->type |= LABIT;	/* confirm, perhaps redundant */
			if (label->type & REDBIT) {
				/* REDBIT meant 'GLOBLBIT' while LABIT was not set. */
				label->type |= EXPBIT;
				label->type &= ~REDBIT;
			}
			if ((mcount | popflags) == 0)
				/* unaccompanied label, display adr like EQU and SET */
				showlabel();
			label = NUL_PTR;	/* reset for next line */
		}
		skipline();
		listline();
		genbin();
		genobj();
		binmbuf = lc += lcjump;
	}
}

PRIVATE void asline()
{
	register struct sym_s *symptr;

	postb = popflags = pcrflag = immcount = lastexp.data = lcjump = 0;
#if SIZEOF_OFFSET_T > 2
	fqflag =
#endif
	    fdflag = fcflag = FALSE;
	cpuwarn();
	readline();
	getsym();
	if (sym != IDENT) {	/* expect label, mnemonic or macro */
		/* Warn if not a comment marker or a hash (for /lib/cpp) */
		if (sym != EOLSYM && sym != IMMEDIATE)
			list_force = TRUE;
		return;		/* anything else is a comment */
	}
	symptr = gsymptr;
	if (!ifflag)
		/* not assembling, just test for IF/ELSE/ELSEIF/ENDIF */
	{
		if (symptr == NUL_PTR || !(symptr->type & MNREGBIT) ||
		    symptr->data & REGBIT ||
		    symptr->value_reg_or_op.op.routine >= MIN_NONCOND)
			return;
	} else if (!(symptr->type & (MACBIT | MNREGBIT)))
		/* not macro, op, pseudo-op or register, expect label */
	{
		oldlabel = symptr->value_reg_or_op.value;

		if ((nocolonlabel = (*lineptr - ':')) == 0) {	/* exported label? */
			sym = COLON;
			++lineptr;
		}
		if (symptr->type & (LABIT | VARBIT)) {
			if (symptr->type & REDBIT)
				labelerror(RELAB);
			label = symptr;

			if (pass
			    && !(symptr->type & VARBIT) /*&& last_pass>1 */ ) {
				label->data = (label->data & FORBIT) | lcdata;
				label->value_reg_or_op.value = lc;
			}
		} else if (checksegrel(symptr)) {
			symptr->type &= ~COMMBIT;	/* ignore COMM, PCOMM gives warning */
#if 0
			if (sym == COLON)
				symptr->type |= EXPBIT;
#endif
			/* remember if forward referenced */
			symptr->data = (symptr->data & FORBIT) | lcdata;
			symptr->value_reg_or_op.value = lc;
			/* unless changed by EQU,COMM or SET */
			label = symptr;
		}

		getsym();
		if (sym != IDENT) {
			if (sym == EQOP) {
				getsym();
				pequ();
			}
			return;	/* anything but ident is comment */
		}
		symptr = gsymptr;
	}
	if (symptr->type & MACBIT) {
		entermac(symptr);
		return;
	}
	if (!(symptr->type & MNREGBIT)) {
		error(OPEXP);
		return;
	}
	if (symptr->data & REGBIT) {
		error(REGUID);
		return;
	}
	mnsize = 0;
	opcode = symptr->value_reg_or_op.op.opcode;
	routine = rout_table[symptr->value_reg_or_op.op.routine];
	getsym();
	(*routine) ();
	if (sym != EOLSYM)
		error(JUNK_AFTER_OPERANDS);
}
