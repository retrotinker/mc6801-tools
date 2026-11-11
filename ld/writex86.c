/* writex86.c - write binary file for linker */

/* Copyright (C) 1994 Bruce Evans */

#include "syshead.h"
#include "x86_aout.h"
#include "const.h"
#include "obj.h"
#include "type.h"
#include "globvar.h"
#include "script.h"

#define page_size() ((bin_off_t)4096)

#ifndef ELF_SYMS
#define ELF_SYMS 0
#endif

#  define FILEHEADERLENGTH (headerless?0:A_MINHDR)
				/* part of header not counted in offsets */
#define DPSEG 2

#define CM_MASK 0xC0
#define MODIFY_MASK 0x3F
#define S_MASK 0x04
#define OF_MASK 0x03

#define CM_SPECIAL 0
#define CM_ABSOLUTE 0x40
#define CM_OFFSET_RELOC 0x80
#define CM_SYMBOL_RELOC 0xC0

#define CM_EOT 0
#define CM_BYTE_SIZE 1
#define CM_WORD_SIZE 2
#define CM_LONG_SIZE 3
#define CM_1_SKIP 17
#define CM_2_SKIP 18
#define CM_4_SKIP 19
#define CM_0_SEG 32

#define ABS_TEXT_MAX 64

#define offsetof(struc, mem) ((size_t) &((struc *) 0)->mem)
#define memsizeof(struc, mem) sizeof(((struc *) 0)->mem)

PRIVATE fastin_t curseg;	/* current segment, 0 to $F */
PRIVATE unsigned nsym;		/* number of symbols written */
PRIVATE unsigned relocsize;	/* current relocation size 1, 2 or 4 */
PRIVATE bin_off_t segadj[NSEG];	/* file offset - address, for the
				   current module's part of each segment */
PRIVATE char segdiscard[NSEG];	/* its part isn't written (NOLOAD) */
PRIVATE bin_off_t segpos[NSEG];	/* segment positions for current module */
PRIVATE bool_t sepid;		/* nonzero for separate I & D */
PRIVATE bool_t stripflag;	/* nonzero to strip symbols */
PRIVATE bin_off_t spos;		/* position in current seg */
PRIVATE bool_t xsym;		/* extended symbol table */

FORWARD void noloadcheck P((struct modstruct * modptr, unsigned *reported));
FORWARD void checkbyte P((struct modstruct * modptr, char *name, int seg,
			  bin_off_t offset, bin_off_t value, int pcrel));
FORWARD void copyname P((char *field, char *name, unsigned size));
FORWARD void writenamerest P((struct nlist * extsym, char *name));
FORWARD void linkmod P((struct modstruct * modptr));
FORWARD void padmod P((struct modstruct * modptr));
FORWARD void setseg P((fastin_pt newseg));
FORWARD void skip P((unsigned countsize));
FORWARD void writeheader P((void));
FORWARD void writescriptsym P((char *name, bin_off_t value, int class));
FORWARD void writenulls P((bin_off_t count));
FORWARD void writefill P((bin_off_t count, int byte));
FORWARD void segwrite P((char *buf, unsigned count));
FORWARD void writeat P((bin_off_t offset, char *buf, unsigned count));

EXTERN bool_t reloc_output;

/* write binary file */

PUBLIC void write_elks(char *outfilename, bool_pt argsepid,
		       bool_pt argstripflag, bool_pt argxsym)
{
	char buf4[4];
	struct nlist extsym;
	flags_t flags;
	struct modstruct *modptr;
	fastin_t seg;
	bin_off_t tempoffset;

	if (reloc_output)
		fatalerror
		    ("Output binformat not configured relocatable, use -N");

	sepid = argsepid;
	stripflag = argstripflag;
	xsym = argxsym;

	/* which imports the script satisfies, and the symbols it defines */
	script_mark_imports();

	/* allocate each loaded module's common blocks, in multiples of 4
	   bytes, and check for undefined symbols */
	for (modptr = modfirst; modptr != NUL_PTR; modptr = modptr->modnext)
		if (modptr->loadflag) {
			register struct symstruct **symparray;
			register struct symstruct *symptr;

			for (symparray = modptr->symparray;
			     (symptr = *symparray) != NUL_PTR; ++symparray)
				if (symptr->modptr == modptr
				    && !(symptr->flags & A_MASK)) {
					if (!(symptr->flags & (I_MASK | SA_MASK)))
						continue;
					if (symptr->value == 0) {
						undefined(symptr->name);
						continue;
					}
					tempoffset = symptr->value;	/* size */
					seg = symptr->flags & SEGM_MASK;
					symptr->value = modptr->comsize[seg];
					modptr->comsize[seg] += tempoffset;
					if (!(symptr->flags & SA_MASK))
						symptr->flags |= C_MASK;
				}
		}

	/* place everything as the linker script says */
	script_layout(sepid, headerless);
	sepid = lay_sepid;

	/* relocate symbols by the addresses of their parts of segments */
	for (modptr = modfirst; modptr != NUL_PTR; modptr = modptr->modnext)
		if (modptr->loadflag) {
			register struct symstruct **symparray;
			register struct symstruct *symptr;

			for (symparray = modptr->symparray;
			     (symptr = *symparray) != NUL_PTR; ++symparray)
				if (symptr->modptr == modptr
				    && !(symptr->flags & A_MASK)) {
					if (symptr->flags & (C_MASK | SA_MASK))
						symptr->value +=
						    modptr->comaddr[symptr->
							    flags & SEGM_MASK];
					else
						symptr->value +=
						    modptr->segaddr[symptr->
							    flags & SEGM_MASK];
				}
		}
	script_define_symbols();

	openout(outfilename);
	writeheader();
	script_write_fill(writeat);
	for (modptr = modfirst; modptr != NUL_PTR; modptr = modptr->modnext)
		if (modptr->loadflag) {
			linkmod(modptr);
			padmod(modptr);
		}
	script_write_data(writeat);

	/* dump symbol table */
	if (!stripflag) {
		seekout(FILEHEADERLENGTH
			+ (unsigned long)(lay_text_end - lay_text_start)
			+ (unsigned long)(lay_data_end - lay_data_start)
		    );
		extsym.n_numaux = extsym.n_type = 0;
		for (modptr = modfirst; modptr != NUL_PTR;
		     modptr = modptr->modnext)
			if (modptr->loadflag) {
				register struct symstruct **symparray;
				register struct symstruct *symptr;

				for (symparray = modptr->symparray;
				     (symptr = *symparray) != NUL_PTR;
				     ++symparray)
					if (symptr->modptr == modptr) {
#if ELF_SYMS
						if (symptr->name[0] == '_'
						    && symptr->name[1])
							strncpy((char *)extsym.
								n_name,
								symptr->name +
								1,
								sizeof extsym.
								n_name);
						else {
							memcpy((char *)extsym.
							       n_name, "$", 1);
							strncpy((char *)extsym.
								n_name + 1,
								symptr->name,
								sizeof(extsym.
								       n_name) -
								1);
						}
#else
						strncpy((char *)extsym.n_name,
							symptr->name,
							sizeof extsym.n_name);
#endif
						u4cn((char *)&extsym.n_value,
						     (u4_t) symptr->value,
						     sizeof extsym.n_value);
						if ((flags =
						     symptr->flags) & A_MASK)
							extsym.n_sclass = N_ABS;
						else if (flags &
							 (E_MASK | I_MASK))
							extsym.n_sclass = C_EXT;
						else
							extsym.n_sclass =
							    C_STAT;
						/* text: counters 0 and 1; bss: common
						   blocks, and counters a script makes
						   NOLOAD (.BSS, in the default layout) */
						if ((!(flags & I_MASK) || flags & C_MASK)
						    && !(flags & A_MASK)) {
							int seg = flags & SEGM_MASK;

							if (flags & (C_MASK | SA_MASK)
							    || symptr->modptr->
							    segnoload[seg])
								extsym.n_sclass |= N_BSS;
							else if (seg < 2)
								extsym.n_sclass |= N_TEXT;
							else
								extsym.n_sclass |= N_DATA;
						}
						writeout((char *)&extsym,
							 sizeof extsym);
						++nsym;
						writenamerest(&extsym, symptr->name);
					}
			}
		/* script symbols that a module imports were written above;
		   write the others, including the segment boundaries that
		   objcopy uses to find the load addresses */
		script_write_symbols(writescriptsym);
		seekout((unsigned long)offsetof(struct exec, a_syms));
		u4cn(buf4, (u4_t) nsym * sizeof extsym,
		     memsizeof(struct exec, a_syms));
		writeout(buf4, memsizeof(struct exec, a_syms));
	}
	closeout();
	if (script_records)
		script_convert(outfilename, entryfirst->elsymptr->value);
	executable();
}

PRIVATE void linkmod(struct modstruct *modptr)
{
	char buf[ABS_TEXT_MAX];
	int command;
	unsigned char modify;
	bin_off_t offset;
	bin_off_t reloffset;
	unsigned reported = 0;	/* counters with NOLOAD data reported */
	int symbolnum;
	struct symstruct **symparray;
	struct symstruct *symptr;

	/* this module's parts of the segments start at their own addresses,
	   and go at their own places in the file */
	for (curseg = 0; curseg < NSEG; ++curseg) {
		segpos[curseg] = modptr->segaddr[curseg];
		segadj[curseg] = modptr->segfadj[curseg];
		segdiscard[curseg] = modptr->segnoload[curseg];
	}
	spos = segpos[curseg = 0];
	if (!segdiscard[curseg])
		seekout(FILEHEADERLENGTH + (unsigned long)spos
			+ (unsigned long)segadj[curseg]);
	relocsize = 2;
	symparray = modptr->symparray;
	openin(modptr->filename);	/* does nothing if already open */
	seekin(modptr->textoffset);
	while (TRUE) {
		if ((command = readchar()) < 0)
			prematureeof();
		modify = command & MODIFY_MASK;
		switch (command & CM_MASK) {
		case CM_SPECIAL:
			switch (modify) {
			case CM_EOT:
				segpos[curseg] = spos;
				return;
			case CM_BYTE_SIZE:
				relocsize = 1;
				break;
			case CM_WORD_SIZE:
				relocsize = 2;
				break;
			case CM_LONG_SIZE:
#ifdef LONG_OFFSETS
				relocsize = 4;
				break;
#else
				fatalerror
				    ("relocation by long offsets not implemented");
#endif
			case CM_1_SKIP:
				skip(1);
				break;
			case CM_2_SKIP:
				skip(2);
				break;
			case CM_4_SKIP:
				skip(4);
				break;
			default:
				if ((modify -= CM_0_SEG) >= NSEG)
					inputerror("bad data in");
				setseg(modify);
				break;
			}
			break;
		case CM_ABSOLUTE:
			noloadcheck(modptr, &reported);
			if (modify == 0)
				modify = ABS_TEXT_MAX;
			readin(buf, (unsigned)modify);
			segwrite(buf, (unsigned)modify);
			spos += (int)modify;
			break;
		case CM_OFFSET_RELOC:
			noloadcheck(modptr, &reported);
			offset = reloffset = readsize(relocsize);
			if (modify & R_MASK)
				offset -= (spos + relocsize);
			offset += modptr->segaddr[modify & SEGM_MASK];
			if (relocsize == 1)
				checkbyte(modptr, (char *)0, modify & SEGM_MASK,
					  reloffset, offset, modify & R_MASK);
			offtocn(buf, offset, relocsize);
			segwrite(buf, relocsize);
			spos += relocsize;
			break;
		case CM_SYMBOL_RELOC:
			noloadcheck(modptr, &reported);
			symptr = symparray[symbolnum = readconvsize((unsigned)
								    (modify &
								     S_MASK ? 2
								     : 1))];
			offset = readconvsize((unsigned)modify & OF_MASK);
			if (modify & R_MASK)
				offset -= (spos + relocsize);
			offset += symptr->value;
			if (relocsize == 1)
				checkbyte(modptr, symptr->name, 0, 0, offset,
					  modify & R_MASK);
			offtocn(buf, offset, relocsize);
			segwrite(buf, relocsize);
			spos += relocsize;
		}
	}
}

PRIVATE void padmod(struct modstruct *modptr)
{
	bin_off_t count;
	fastin_t seg;
	bin_off_t size;
	unsigned sizecount;
	char *sizeptr;

	for (seg = 0, sizeptr = modptr->segsize; seg < NSEG; ++seg) {
		size = cntooffset(sizeptr,
				  sizecount =
				  segsizecount((unsigned)seg, modptr));
		sizeptr += sizecount;
		if ((count = segpos[seg] - modptr->segaddr[seg]) != size)
			size_error(seg, count, size);

		/* pad as the layout decided (SUBALIGN) */
		/* not padding in-between common areas which sometimes get into file */
		if ((size = modptr->segpad[seg]) != 0) {
			setseg(seg);
			writefill(size, modptr->segfill[seg]);
			segpos[seg] = spos;
		}
	}
}

/* write a symbol defined by the linker script */

PRIVATE void writescriptsym(char *name, bin_off_t value, int class)
{
	struct nlist extsym;
	int sclass;

	if (class == SCRIPT_TEXT)
		sclass = C_EXT | N_TEXT;
	else if (class == SCRIPT_DATA)
		sclass = C_EXT | N_DATA;
	else
		sclass = N_ABS;
	memset((void *)&extsym, 0, sizeof extsym);
	copyname(extsym.n_name, name, sizeof extsym.n_name);
	u4cn((char *)&extsym.n_value, (u4_t) value, sizeof extsym.n_value);
	extsym.n_sclass = sclass;
	writeout((char *)&extsym, sizeof extsym);
	++nsym;
	writenamerest(&extsym, name);
}

/* in an extended symbol table (-y, the default), the rest of a name
   longer than a symbol entry's field goes in entries of its own, with
   class 0, after the symbol's entry */

PRIVATE void writenamerest(struct nlist *extsym, char *name)
{
	unsigned i, len = strlen(name);

	if (!xsym)
		return;
	extsym->n_sclass = 0;
	memset((void *)&extsym->n_value, 0, sizeof extsym->n_value);
	for (i = sizeof extsym->n_name; i < len; i += sizeof extsym->n_name) {
		copyname(extsym->n_name, name + i, sizeof extsym->n_name);
		writeout((char *)extsym, sizeof *extsym);
		++nsym;
	}
}

/* set new segment */

PRIVATE void setseg(fastin_pt newseg)
{
	if (newseg != curseg) {
		segpos[curseg] = spos;
		spos = segpos[curseg = newseg];
		if (!segdiscard[curseg])
			seekout(FILEHEADERLENGTH + (unsigned long)spos
				+ (unsigned long)segadj[curseg]);
	}
}

PRIVATE void skip(unsigned countsize)
{
	writenulls((bin_off_t) readsize(countsize));
}

PRIVATE void writeheader(void)
{
	struct exec header;
	bin_off_t heaptop;

	memset(&header, 0, sizeof header);
	header.a_magic[0] = A_MAGIC0;
	header.a_magic[1] = A_MAGIC1;
	header.a_flags = sepid ? A_SEP : A_EXEC;
	header.a_cpu = A_M6801;
	header.a_hdrlen = FILEHEADERLENGTH;
	offtocn((char *)&header.a_text, lay_text_end - lay_text_start,
		sizeof header.a_text);
	offtocn((char *)&header.a_data, lay_data_end - lay_data_start,
		sizeof header.a_data);
	offtocn((char *)&header.a_bss, lay_bss_end - lay_data_end,
		sizeof header.a_bss);
	offtocn((char *)&header.a_entry, entryfirst->elsymptr->value,
		sizeof header.a_entry);

	/* total memory: up to __heap_top if the script sets it */
	if (!script_value("__heap_top", &heaptop))
		heaptop = lay_bss_end + heap_top_value;
	offtocn((char *)&header.a_total,
		heaptop - lay_data_start + lay_text_end - lay_text_start,
		sizeof header.a_total);
	if (FILEHEADERLENGTH)
		writeout((char *)&header, FILEHEADERLENGTH);
}

/* write module data, unless its part of the segment is NOLOAD */
PRIVATE void segwrite(char *buf, unsigned count)
{
	if (!segdiscard[curseg])
		writeout(buf, count);
}

/* write bytes for the linker script at an offset after the header */
PRIVATE void writeat(bin_off_t offset, char *buf, unsigned count)
{
	seekout(FILEHEADERLENGTH + (unsigned long)offset);
	writeout(buf, count);
}

/* initialized data in the current counter, if a script puts it in a
   NOLOAD section, would be lost: report it, once per counter */

PRIVATE void noloadcheck(struct modstruct *modptr, unsigned *reported)
{
	if (segdiscard[curseg] && !(*reported & (1U << curseg))) {
		*reported |= 1U << curseg;
		noload_data(modptr->filename, modptr->archentry, curseg,
			    modptr->segsect[curseg]);
	}
}

/* check that a one-byte relocation at spos fits: a direct address or
   indexed offset in $00-$FF, or a branch within -128 to +127 */

PRIVATE void checkbyte(struct modstruct *modptr, char *name, int seg,
		       bin_off_t offset, bin_off_t value, int pcrel)
{
	if (pcrel ? value < -0x80 || value > 0x7F : value < 0 || value > 0xFF)
		byte_reloc_error(modptr->filename, modptr->archentry, name, seg,
				 offset, value, spos, pcrel);
}

/* copy a name into an a.out name field, padding it with nulls; a name
   that fills the field isn't terminated */

PRIVATE void copyname(char *field, char *name, unsigned size)
{
	unsigned len = strlen(name);

	if (len > size)
		len = size;
	memcpy(field, name, len);
	memset(field + len, 0, size - len);
}

PRIVATE void writefill(bin_off_t count, int byte)
{
	spos += count;
	if (segdiscard[curseg])
		return;
	while (count-- > 0)
		writechar(byte);
}

PRIVATE void writenulls(bin_off_t count)
{
	long lcount = count;
	spos += count;
	if (segdiscard[curseg])
		return;
#if 0
	/* This will only work if we record the highest spos found an seek there
	 * at the end of the generation
	 */

	seekout(FILEHEADERLENGTH + (unsigned long)spos
		+ (unsigned long)segadj[curseg]);
	return;

#endif
	if (lcount < 0)
		fatalerror("org command requires reverse seek");
	while (count-- > 0)
		writechar(0);
}
