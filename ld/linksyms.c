
/* linksyms.c - write binary file for linker */

/* Copyright (C) 1994 Bruce Evans */

#include "syshead.h"
#include "const.h"
#include "obj.h"
#include "type.h"
#undef EXTERN
#include "globvar.h"
#include "script.h"

FORWARD void linkrefs P((struct modstruct * modptr));
FORWARD bool_pt iscommon P((struct symstruct * symptr));
PUBLIC bool_t reloc_output = 0;

/* link all symbols connected to entry symbols */

PUBLIC void linksyms(bool_pt argreloc_output)
{
	char needlink;
	struct entrylist *elptr;
	struct modstruct *modptr;
	struct symstruct *symptr;

#ifdef REL_OUTPUT
	reloc_output = argreloc_output;
	if (argreloc_output) {
		if (modfirst->modnext != NUL_PTR)
			fatalerror
			    ("relocatable output only works for one input file");
		for (modptr = modfirst; modptr != NUL_PTR;
		     modptr = modptr->modnext)
			modptr->loadflag = TRUE;
		return;
	}
#endif
	if ((symptr = findsym("_main")) != NUL_PTR)
		entrysym(symptr);
	do {
		if ((elptr = entryfirst) == NUL_PTR)
			fatalerror("no start symbol");
		for (modptr = modfirst; modptr != NUL_PTR;
		     modptr = modptr->modnext)
			modptr->loadflag = FALSE;
		for (; elptr != NUL_PTR; elptr = elptr->elnext)
			linkrefs(elptr->elsymptr->modptr);
		/* modules the linker script keeps or uses in ASSERTs */
		script_roots(linkrefs);
		if ((symptr = findsym("start")) != NUL_PTR)
			linkrefs(symptr->modptr);
		needlink = FALSE;
		{
			struct redlist *prlptr = 0;
			struct redlist *rlptr;

			for (rlptr = redfirst; rlptr != NUL_PTR;
			     rlptr = (prlptr = rlptr)->rlnext)
				if (rlptr->rlmodptr->loadflag &&
				    rlptr->rlmodptr->class >
				    rlptr->rlsymptr->modptr->class) {
					rlptr->rlsymptr->modptr =
					    rlptr->rlmodptr;
					rlptr->rlsymptr->value = rlptr->rlvalue;
					if (rlptr == redfirst)
						redfirst = rlptr->rlnext;
					else
						prlptr->rlnext = rlptr->rlnext;
					needlink = TRUE;
				}
		}
	}
	while (needlink);
	/* a common block whose module isn't linked (it was given to the
	   first with the largest size) goes to a linked module that has
	   it, which allocates it */
	for (modptr = modfirst; modptr != NUL_PTR; modptr = modptr->modnext)
		if (modptr->loadflag) {
			struct symstruct **symparray;
			char *comflags;

			for (symparray = modptr->symparray,
			     comflags = modptr->comflags;
			     (symptr = *symparray) != NUL_PTR;
			     ++symparray, ++comflags)
				if (*comflags && iscommon(symptr)
				    && !symptr->modptr->loadflag)
					symptr->modptr = modptr;
		}
}

/* a symbol that only common blocks have (no module defines it) */
PRIVATE bool_pt iscommon(struct symstruct *symptr)
{
	return (symptr->flags & (E_MASK | I_MASK)) == I_MASK
	    && symptr->value != 0;
}

PRIVATE void linkrefs(struct modstruct *modptr)
{
	register struct symstruct **symparray;
	register struct symstruct *symptr;
	register char *comflags;

	modptr->loadflag = TRUE;
	/* (not the module a common block was given to, for a common block
	   here: one of the same name elsewhere is no reason to link it) */
	for (symparray = modptr->symparray, comflags = modptr->comflags;
	     (symptr = *symparray) != NUL_PTR; ++symparray, ++comflags)
		if (symptr->modptr->loadflag == FALSE
		    && !(*comflags && iscommon(symptr)))
			linkrefs(symptr->modptr);
}
