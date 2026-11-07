/*
 * This is a combination of three tools for decoding information from
 * Dev86/ELKS object files and executables.
 *
 * This executable can be given the names:
 *
 *  objdump86: Dumps detailed information about a binary file.
 *  size86:    Summary sizes of the data in a binary file.
 *  nm86:      The symbol table of the binary file.
 *
 * None of these programs have any options.
 * This may be a minor problem with nm86.
 *
 * Copyright (c) 1999 by Greg Haerr <greg@censoft.com>
 * Added archive file reading capabilties
 */

#include <stdio.h>
#ifdef __STDC__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <string.h>
#include "const.h"
#include "ar.h"
#include "obj.h"
#include "m6801ops.h"

FILE *ifd;
char *ifname;
char *progname;			/* as invoked: objdump01, nm01 or size01 */

#ifdef __STDC__
#define _(x) x
#else
#define _(x) ()
#endif

long get_long _((void));
long get_sized _((int sz));
unsigned int get_word _((void));
int get_byte _((void));
int main _((int argc, char **argv));
void do_file _((char *fname));
long read_arheader _((char *archentry));
void do_module _((char *fname, char *archive));
int error _((char *str));
int read_objheader _((void));
int read_sectheader _((void));
int read_syms _((void));
void disp_sectheader _((void));
int disp_syms _((void));
void read_databytes _((void));
void hex_output _((int ch));
void fetch_aout_hdr _((void));
void dump_aout _((void));
void dstart _((void));
void dbyte _((int seg, long pos, int ch));
void drel _((int seg, long pos, int size, char *text));
void dfree _((void));
void disasm_aout _((void));
void disasm_module _((void));
char *dlabel _((int seg, long addr, int textonly));
char *dnearest _((int seg, long addr));
struct dcode;
void disassemble _((struct dcode *dc));
void size_aout _((void));
void nm_aout _((void));
int m6801_symletter _((unsigned int symtype, long offset));

int obj_ver;
int sections;
long segsizes[16];
long textoff;
long textlen;
long str_off;
long str_len;
long filepos;
int num_syms;
long code_bytes;

char **symnames;
char *strtab;

struct {
	unsigned int nameoff, symtype;
	long offset;
} *symtab;

int display_mode = 0;
int multiple_files = 0;
int exit_status = 0;		/* 1 once any error has been reported */
int byte_order = 0;
int m6801 = 0;			/* object file is from as01, not as86 */

int opt_o;
int opt_d;			/* -d: disassemble code too */

long size_text, size_data, size_bss;
long tot_size_text = 0, tot_size_data = 0, tot_size_bss = 0;

int main(int argc, char **argv)
{
	int ar;
	char *p;

	ifd = stdin;		/* In Libc6 stdin is weird */
	m6801_init_ops();

	p = strrchr(argv[0], '/');
	if (p)
		p++;
	else
		p = argv[0];
	progname = p;

	if (p[0] == 's')
		display_mode = 1;
	if (p[0] == 'n')
		display_mode = 2;

	multiple_files = 0;
	for (ar = 1; ar < argc; ar++) {
		if (argv[ar][0] == '-')
			switch (argv[ar][1]) {
			case 's':
				display_mode = 1;
				break;
			case 'n':
				display_mode = 2;
				break;
			case 'o':
				opt_o++;
				break;
			case 'd':
				opt_d++;
				break;
		} else
			multiple_files++;
	}

	if (!multiple_files)
		exit(1);

	multiple_files = (multiple_files > 1);

	if (display_mode == 1)
		printf("text\tdata\tbss\tdec\thex\tfilename\n");

	for (ar = 1; ar < argc; ar++)
		if (argv[ar][0] != '-')
			do_file(argv[ar]);

	if (display_mode == 1 && multiple_files)
		printf("%ld\t%ld\t%ld\t%ld\t%lx\tTotal\n",
		       tot_size_text, tot_size_data, tot_size_bss,
		       tot_size_text + tot_size_data + tot_size_bss,
		       tot_size_text + tot_size_data + tot_size_bss);

	return exit_status;
}

void do_file(char *fname)
{
	int c0, c1;
	long filelength;
	char archentry[sizeof(struct ar_hdr)];	/* sizeof ar_hdr.ar_name */
	char filemagic[SARMAG];

	ifname = fname;
	if ((ifd = fopen(fname, "rb")) == 0) {
		error("Cannot open file");
		return;
	}
	filepos = 0L;

	/* check if file is an archive */
	if (fread(filemagic, sizeof(filemagic), 1, ifd) == 1
	    && strncmp(filemagic, ARMAG, sizeof filemagic) == 0) {
		filepos = SARMAG;
		while ((filelength = read_arheader(archentry)) > 0) {
			filepos += sizeof(struct ar_hdr);
			c0 = getc(ifd);
			c1 = getc(ifd);
			if ((c0 == 0xA3 && c1 == 0x86)	/* as86 OMAGIC */
			    || (c0 == 'S' && c1 == '1')) {	/* as01 OMAGIC */
				fseek(ifd, filepos, 0);
				do_module(archentry, fname);
			} else if (c0 == '!' && c1 == '<')
				filelength = SARMAG;
			filepos += ld_roundup(filelength, 2, long);
			fseek(ifd, filepos, 0);
		}
	} else {
		fseek(ifd, 0L, 0);
		do_module(fname, NULL);
	}
	fclose(ifd);
}

/* read archive header and return length */
long read_arheader(char *archentry)
{
	char *endptr;
	struct ar_hdr arheader;

	if (fread((char *)&arheader, sizeof(arheader), 1, ifd) != 1)
		return 0;
	memcpy(archentry, arheader.ar_name, sizeof(arheader.ar_name));
	archentry[sizeof(arheader.ar_name)] = 0;
	endptr = archentry + sizeof(arheader.ar_name);
	do {
		*endptr-- = 0;
	} while (endptr > archentry && (*endptr == ' ' || *endptr == '/'));
	return strtoul(arheader.ar_size, (char **)NULL, 0);
}

void do_module(char *fname, char *archive)
{
	int modno, i;

	size_text = size_data = size_bss = 0;
	byte_order = 0;

	if (!display_mode) {
		if (archive)
			printf("ARCHIVEFILE '%s'\n", archive);
		printf("OBJECTFILE '%s'\n", fname);
	}

	switch (read_objheader()) {
	case 0:		/* as86 object file */

		for (modno = 1; modno <= sections; modno++) {
			if (modno != 1 && !display_mode)
				printf("OBJECTSECTION %d\n", modno);
			if (read_sectheader() < 0)
				break;

			/* as86: segments 0, 4-E are text, 1-3 are data */
			/* as01: segments 0-1 are text, 4 (.BSS) is
			   uninitialized data, and the rest are data */
			for (i = 0; i < 16; i++) {
				if (m6801 ? i < 2 : (i < 1 || i > 4))
					size_text += segsizes[i];
				else if (m6801 && i == 4)
					size_bss += segsizes[i];
				else
					size_data += segsizes[i];
			}

			if (read_syms() < 0)
				break;

			strtab = malloc((unsigned int)str_len + 1);
			if (strtab == 0) {
				error("Out of memory");
				break;
			}
			str_off = ftell(ifd);
			if (fread(strtab, 1, (unsigned int)str_len, ifd)
			    != (size_t)str_len) {
				error("Cannot read string table");
				break;
			}
			strtab[str_len] = 0;

			disp_sectheader();
			disp_syms();

			if (display_mode == 0)
				printf("text\tdata\tbss\tdec\thex\tfilename\n");
			if (display_mode != 2) {
				printf("%ld\t%ld\t%ld\t%ld\t%lx\t",
				       size_text, size_data, size_bss,
				       size_text + size_data + size_bss,
				       size_text + size_data + size_bss);

				if (archive)
					printf("%s(%s)\n", archive, fname);
				else
					printf("%s\n", fname);

				tot_size_text += size_text;
				tot_size_data += size_data;
				tot_size_bss += size_bss;
			}

			if (sections == 1 && display_mode != 0)
				break;

			read_databytes();
		}
		break;

	case 1:		/* ELKS executable */
		fseek(ifd, 0L, 0);
		fetch_aout_hdr();

		switch (display_mode) {
		case 0:
			dump_aout();
			break;
		case 1:
			size_aout();
			break;
		case 2:
			nm_aout();
			break;
		}
		break;
	}

	if (strtab)
		free(strtab);
	if (symnames)
		free(symnames);
	strtab = 0;
	symnames = 0;
}

int error(char *str)
{
	exit_status = 1;
	switch (display_mode) {
	case 1:
	case 2:
		fprintf(stderr, "%s: %s: %s\n", progname, ifname, str);
		break;
	default:
		printf("Error: %s\n", str);
		break;
	}
	return -1;
}

int read_objheader(void)
{
	unsigned char buf[5];

	if (fread(buf, 1, 5, ifd) != 5)
		return error("Cannot read object header");

	if (buf[0] == 'S' && buf[1] == '1') {
		/* as01 object file: same layout, but big-endian */
		m6801 = 1;
		byte_order = 3;
	} else if (buf[0] == 0xA3 && buf[1] == 0x86) {
		m6801 = 0;
	} else {
		if (buf[0] == 1 && buf[1] == 3) {
			sections = 1;
			return 1;
		}
		return error("Bad magic number");
	}

	if ((unsigned char)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4])
		return error("Bad header checksum");

	if (m6801)
		sections = 256 * buf[2] + buf[3];
	else
		sections = buf[2] + 256 * buf[3];

	return 0;
}

int read_sectheader(void)
{
	unsigned char ssdesc[4];
	int i;

	textoff = get_long();	/* Offset of bytecode in file */
	textlen = get_long();	/* Length of text+data (no bss) in memory */
	str_len = get_word();	/* Length of string table in file */
	obj_ver = get_word();	/* 0.0 */

	(void)get_long();	/* Ignore fives */
	for (i = 0; i < 4; i++)	/* Sixteen segment size sizes */
		ssdesc[i] = get_byte();

	for (i = 0; i < 16; i++) {
		int ss;
		ss = 3 & (ssdesc[(15 - i) / 4] >> (2 * (i % 4)));
		segsizes[i] = get_sized(ss);
	}

	num_syms = get_word();	/* Number of symbol codes */

	return 0;
}

void disp_sectheader(void)
{
	int i;
	if (display_mode)
		return;

	printf("MODULE  '%s'\n", strtab);
	printf("BYTEPOS %08lx\n", textoff);
	printf("BINLEN  %08lx\n", textlen);
	printf("STRINGS %04lx +%04lx\n", str_off, str_len);
	printf("VERSION %d.%d\n", obj_ver / 256, obj_ver % 256);

	for (i = 0; i < 16; i++)
		if (segsizes[i])
			printf("SEG%x %08lx\n", i, segsizes[i]);

	printf("\n");
	printf("SYMS %u\n", num_syms);
}

int read_syms(void)
{
	int i;

	if (num_syms < 0)
		return error("Bad symbol table");

	symnames = malloc(num_syms * sizeof(char *) + 1);
	if (symnames == 0)
		return error("Out of memory");

	symtab = calloc(num_syms, sizeof(*symtab));
	if (symtab == 0)
		return error("Out of memory");

	for (i = 0; i < num_syms; i++) {
		unsigned int symtype;

		symtab[i].nameoff = get_word();
		symtab[i].symtype = get_word();
		symtype = (symtab[i].symtype & 0x3FFF);

		if (symtab[i].nameoff == -1 || symtab[i].symtype == -1) {
			printf("!!! EOF in symbol table\n");
			break;
		}
		symtab[i].offset = get_sized((symtab[i].symtype >> 14) & 3);

		if (m6801) {
			/* common (imported with a size) or local common */
			if ((symtype & 0x40 && symtab[i].offset)
			    || symtype & 0x2000)
				size_bss += symtab[i].offset;
		} else if (symtype == 0x43 || symtype == 0x2003)
			size_bss += symtab[i].offset;
	}

	return 0;
}

int disp_syms(void)
{
	int i;

	if (display_mode == 2 && multiple_files && !opt_o)
		printf("\n%s:\n", ifname);

	for (i = 0; i < num_syms; i++) {
		long offset = 0;
		unsigned int nameoff, symtype;

		nameoff = symtab[i].nameoff;
		symtype = symtab[i].symtype;
		offset = symtab[i].offset;

		symtype &= 0x3FFF;
		if (nameoff > str_len || nameoff < 0)
			symnames[i] = strtab + str_len;
		else
			symnames[i] = strtab + nameoff;

		if (!display_mode) {
			printf("SYM %-4d %08lx ", i, offset);

			printf("%s", (symtype & 0x2000) ? "C" : ".");
			printf("%s", (symtype & 0x0100) ? "N" : ".");
			printf("%s", (symtype & 0x0080) ? "E" : ".");
			printf("%s", (symtype & 0x0040) ? "I" : ".");
			printf("%c",
			       "T12D456789abcdeUAhijklmnopqrstuv"[symtype &
								  0x1F]);
			if (symtype & 0x1E20)
				printf(" %04x", symtype);
			printf(" %s\n", symnames[i]);
		}
		if (display_mode == 2 && m6801) {
			int letter = m6801_symletter(symtype, offset);

			if (opt_o)
				printf("%s: ", ifname);
			if (letter == 'U')
				printf("         ");
			else
				printf("%08lx ", offset);
			printf("%c %s\n", letter, symnames[i]);
		} else if (display_mode == 2) {
			if (opt_o)
				printf("%s: ", ifname);
			if (symtype == 0x004f || symtype == 0x0040)
				printf("         ");
			else
				printf("%08lx ", offset);
			switch (symtype) {
			case 0x004F:
				putchar('U');
				break;
			case 0x0000:
				putchar('t');
				break;
			case 0x0003:
				putchar('d');
				break;
			case 0x2003:
				putchar('b');
				break;
			case 0x0043:
				putchar('C');
				break;
			case 0x0083:
				putchar('D');
				break;
			case 0x0080:
				putchar('T');
				break;
			case 0x0040:
				putchar('T');
				break;
			case 0x0180:
				putchar('N');
				break;
			case 0x0010:
				putchar('a');
				break;
			case 0x0090:
				putchar('A');
				break;
			default:
				if ((symtype & ~0xF) == 0x40)
					putchar('u');
				else if ((symtype & ~0xF) == 0x80)
					printf("%c",
					       "T12D456789abcdeU"[symtype &
								  0xF]);
				else
					putchar('?');
				break;
			}
			printf(" %s\n", symnames[i]);
		}
	}
	if (!display_mode)
		printf("\n");

	return 0;
}

/* nm symbol letter for an as01 object file symbol, in the same style as
   for executables: upper case for exported symbols */
int m6801_symletter(unsigned int symtype, long offset)
{
	int letter;

	if (symtype & 0x40)	/* imported: common if it has a size */
		return offset ? 'C' : 'U';
	if (symtype & 0x2000)	/* local common */
		letter = 'b';
	else if (symtype & 0x10)	/* absolute */
		letter = 'a';
	else if ((symtype & 0xF) < 2)	/* segments 0-1 are text */
		letter = 't';
	else if ((symtype & 0xF) == 4)	/* .BSS */
		letter = 'b';
	else
		letter = 'd';
	if (symtype & 0x80)	/* exported */
		letter -= 'a' - 'A';
	return letter;
}

void read_databytes(void)
{
	static char *relstr[] = { "ERR", "DB", "DW", "DD" };
	long l, cpos;
	int ch, i;
	int curseg = 0;
	int relsize = 0;
	long pos[16];		/* for -d: offset in each location counter */
	int dis = opt_d && m6801;
	char rtext[100];

	cpos = ftell(ifd);
	fseek(ifd, filepos + textoff, 0);
	memset(pos, 0, sizeof pos);
	if (dis)
		dstart();

	printf("\nBYTECODE\n");
	for (;;) {
		if ((ch = get_byte()) == -1)
			break;

		if (ch == 0)
			break;

		switch (ch & 0xC0) {
		case 0x00:
			switch (ch & 0xF0) {
			case 0x00:	/* Relocator size */
				printf("RELSZ %d\n", relsize = (ch & 0xF));
				if (relsize > 3)
					relsize = 3;
				break;
			case 0x10:	/* Skip bytes */
				printf("SKP %ld\n", l = get_sized(ch & 0xF));
				pos[curseg] += l;
				break;
			case 0x20:	/* Segment */
				printf("SEG %x\n", curseg = (ch & 0xF));
				break;
			default:
				printf("CODE %02x - unknown\n", ch);
				goto break_break;
			}
			break;
		case 0x40:	/* Raw bytes */
			{
				int abscnt = (ch & 0x3F);
				if (abscnt == 0)
					abscnt = 64;
				for (i = 0; i < abscnt; i++) {
					if ((ch = get_byte()) == -1)
						break;
					hex_output(ch);
					if (dis)
						dbyte(curseg, pos[curseg], ch);
					pos[curseg]++;
				}
				hex_output(EOF);

				if (ch == -1)
					goto break_break;
			}
			break;
		case 0x80:	/* Relocator - simple */
			l = get_sized(relsize);
			printf("%s SEG%x%s%s", relstr[relsize],
			       (ch & 0xF),
			       (ch & 0x20) ? "-PC" : "",
			       (ch & 0x10) ? "+?" : "");
			if (l)
				printf("+0x%04lx", l);
			putchar('\n');
			if (dis) {	/* a label in counter ch & 0xF, if any */
				char *name = dnearest(ch & 0xF, l);

				if (name)
					sprintf(rtext, "%.80s", name);
				else
					sprintf(rtext, "SEG%x+$%04lx", ch & 0xF, l);
				if (ch & 0x20)
					strcat(rtext, "-PC");
				drel(curseg, pos[curseg], relsize == 3 ? 4
				     : relsize, rtext);
			}
			pos[curseg] += relsize == 3 ? 4 : relsize;
			break;
		case 0xC0:	/* Relocator - symbol relative */
			if (ch & 0x18) {
				printf("CODE %02x - unknown\n", ch);
				goto break_break;
			}
			if (ch & 4)
				i = get_word();
			else
				i = get_byte();
			l = get_sized(ch & 3);

			printf("%s %s%s%s", relstr[relsize],
			       symnames[i],
			       (ch & 0x20) ? "-PC" : "",
			       (ch & 0x18) ? "+?" : "");
			if (l)
				printf("+0x%04lx", l);
			putchar('\n');
			if (dis) {
				sprintf(rtext, "%.80s", symnames[i]);
				if (l)
					sprintf(rtext + strlen(rtext), "+$%lx", l);
				if (ch & 0x20)
					strcat(rtext, "-PC");
				drel(curseg, pos[curseg], relsize == 3 ? 4
				     : relsize, rtext);
			}
			pos[curseg] += relsize == 3 ? 4 : relsize;
			break;
		}
	}
break_break:;
	printf("\n");
	if (dis) {
		disasm_module();
		dfree();
	}
	fseek(ifd, cpos, 0);
}

long get_sized(int sz)
{
	switch (sz) {
	case 0:
		return 0;
	case 1:
		return get_byte();
	case 2:
		return get_word();
	case 3:
		return get_long();
	}
	return -1;
}

long get_long(void)
{
	long retv = 0;
	int i;
	for (i = 0; i < 32; i += 16) {
		unsigned int v = get_word();
		if (byte_order & 2)
			retv += ((long)v << (16 - i));
		else
			retv += ((long)v << i);
	}
	return retv;
}

unsigned int get_word(void)
{
	long retv = 0;
	int i;
	for (i = 0; i < 16; i += 8) {
		int v = getc(ifd);
		if (v == EOF)
			return -1;
		code_bytes++;
		if (byte_order & 1)
			retv += (v << (8 - i));
		else
			retv += (v << i);
	}
	return retv;
}

int get_byte(void)
{
	int v = getc(ifd);
	if (v == EOF)
		return -1;
	code_bytes++;
	return v;
}

void hex_output(int ch)
{
	static char linebuf[80];
	static char buf[20];
	static int pos = 0;

	if (ch == EOF) {
		if (pos)
			printf(": %.66s\n", linebuf);
		pos = 0;
	} else {
		if (!pos)
			memset(linebuf, ' ', sizeof(linebuf));
		sprintf(buf, "%02x", ch & 0xFF);
		memcpy(linebuf + pos * 3 + (pos > 7), buf, 2);
		if (ch > ' ' && ch <= '~')
			linebuf[50 + pos] = ch;
		else
			linebuf[50 + pos] = '.';
		pos = ((pos + 1) & 0xF);
		if (pos == 0) {
			printf(": %.66s\n", linebuf);
			memset(linebuf, ' ', sizeof(linebuf));
		}
	}
}

/************************************************************************/
/* ELKS a.out versions
 */

long header[12];
int h_len, h_flgs, h_cpu;

void fetch_aout_hdr(void)
{
	int i;

	header[0] = get_long();
	header[1] = get_long();
	byte_order = ((header[0] >> 24) & 3);

	h_len = (header[1] & 0xFF);
	h_flgs = ((header[0] >> 16) & 0xFF);
	h_cpu = ((header[0] >> 24) & 0xFF);

	for (i = 2; i < 8; i++) {
		if (i * 4 <= h_len)
			header[i] = get_long();
		else
			header[i] = 0;
	}
}

void dump_aout(void)
{
	static char *cpu[] =
	    { "unknown", "8086", "m68k", "ns16k", "i386", "sparc", "m6809",
	      "m6801" };
	static char *byteord[] =
	    { "LITTLE_ENDIAN", "(2143)", "(3412)", "BIG_ENDIAN" };
	int i;
	long l;

	if (h_cpu > 0x1F)
		h_cpu &= 3;

	printf("HLEN %d\n", h_len);
	printf("CPU  %s %s\n", cpu[h_cpu >> 2], byteord[h_cpu & 3]);

	printf("FLAGS:");
	if (h_flgs & 0x01)
		printf(" FLG-01");
	if (h_flgs & 0x02)
		printf(" FLG-02");
	if (h_flgs & 0x04)
		printf(" A_NSYM");
	if (h_flgs & 0x08)
		printf(" FLG-08");
	if (h_flgs & 0x10)
		printf(" A_EXEC");
	if (h_flgs & 0x20)
		printf(" A_SEP");
	if (h_flgs & 0x40)
		printf(" A_PURE");
	if (h_flgs & 0x80)
		printf(" A_TOVLY");
	printf("\n");

	if (header[5])
		printf("a_entry  = 0x%08lx\n", header[5]);
	printf("a_total  = 0x%08lx\n", header[6]);
	if (header[7])
		printf("a_syms   = 0x%08lx\n", header[7]);

	if (h_len >= 36)
		printf("a_trsize = 0x%08lx\n", header[8]);
	if (h_len >= 40)
		printf("a_drsize = 0x%08lx\n", header[9]);
	if (h_len >= 44)
		printf("a_tbase  = 0x%08lx\n", header[10]);
	if (h_len >= 48)
		printf("a_dbase  = 0x%08lx\n", header[11]);
	printf("\n");

	size_aout();
	printf("\n");

	if (header[7]) {
		printf("SYMBOLS\n");
		nm_aout();
	} else
		printf("NO SYMBOLS\n");
	printf("\n");

	printf("TEXTSEG\n");
	fseek(ifd, (long)h_len, 0);
	for (l = 0; l < header[2]; l++) {
		if ((i = getc(ifd)) == EOF)
			break;
		hex_output(i);
	}
	hex_output(EOF);

	printf("DATASEG\n");
	fseek(ifd, (long)h_len + header[2], 0);
	for (l = 0; l < header[3]; l++) {
		if ((i = getc(ifd)) == EOF)
			break;
		hex_output(i);
	}
	hex_output(EOF);

	if (opt_d) {
		printf("\n");
		disasm_aout();
	}
}

void size_aout(void)
{
	if (display_mode == 0)
		printf("text\tdata\tbss\tdec\thex\tfilename\n");

	printf("%ld\t%ld\t%ld\t%ld\t%lx\t%s\n",
	       header[2], header[3], header[4],
	       header[2] + header[3] + header[4],
	       header[2] + header[3] + header[4], ifname);

	tot_size_text += header[2];
	tot_size_data += header[3];
	tot_size_bss += header[4];
}

void nm_aout(void)
{
	char n_name[10];
	long n_value;
	int n_sclass, n_numaux, n_type;
	long bytes_left;
	int pending_nl = 0;

	fseek(ifd, h_len + header[2] + header[3] + header[8] + header[9], 0);

	if (h_flgs & 4) {
		error("Executable has new format symbol table");
		return;
	}

	bytes_left = header[7];

	if (bytes_left == 0)
		printf("No symbols in '%s'\n", ifname);
	else if (multiple_files && !opt_o)
		printf("\n%s:\n", ifname);

	while (bytes_left > 16) {
		if (fread(n_name, 1, 8, ifd) != 8)
			break;
		n_name[8] = 0;
		n_value = get_long();
		if ((n_sclass = getc(ifd)) == EOF)
			break;
		if ((n_numaux = getc(ifd)) == EOF)
			break;
		n_type = get_word();

		if (pending_nl && n_sclass == 0) {
			printf("%s", n_name);
			continue;
		}

		if (pending_nl)
			putchar('\n');
		if (opt_o)
			printf("%s: ", ifname);
		if (n_sclass == 0x10)
			printf("         ");
		else
			printf("%08lx ", n_value);
		switch (n_sclass) {
		case 0x01:
			printf("a ");
			break;
		case 0x12:
			printf("T ");
			break;
		case 0x13:
			printf("D ");
			break;
		case 0x14:
			printf("C ");
			break;
		case 0x1a:
			printf("t ");
			break;
		case 0x1b:
			printf("d ");
			break;
		case 0x1c:
			printf("b ");
			break;
		case 0x10:
			printf("U ");
			break;
		default:
			if (display_mode) {
				printf("? ");
				break;
			}

			printf("n_sclass=");
			switch (n_sclass >> 3) {
			case 0:
				printf("C_NULL,");
				break;
			case 2:
				printf("C_EXT,");
				break;
			case 3:
				printf("C_STAT,");
				break;
			default:
				printf("%04o,", n_sclass & 0xF8);
			}
			switch (n_sclass & 7) {
			case 0:
				printf("N_UNDF ");
				break;
			case 1:
				printf("N_ABS ");
				break;
			case 2:
				printf("N_TEXT ");
				break;
			case 3:
				printf("N_DATA ");
				break;
			case 4:
				printf("N_BSS ");
				break;
			case 5:
				printf("N_COMM ");
				break;
			default:
				printf("%o ", n_sclass & 7);
				break;
			}
			break;
		}

		if (display_mode == 0) {
			if (n_numaux)
				printf("n_numaux=%02x ", n_numaux);
			if (n_type)
				printf("n_type=%04x ", n_type);
		}

		printf("%s", n_name);

		pending_nl = 1;
	}

	if (pending_nl)
		putchar('\n');
}

/************************************************************************/
/* Disassembly (-d)
 */

/* a relocated field in code: its size in bytes, and its target as an
   operand */
struct dreloc {
	int size;
	char text[80];
};

/* code to disassemble: location counter seg of an object module (with
   its relocations), or an executable's text segment (seg -1) */
struct dcode {
	unsigned char *bytes;
	char *have;		/* the byte was loaded, not skipped */
	struct dreloc **rel;	/* the relocation starting at each byte */
	long len;
	long base;		/* the address of the first byte */
	int seg;
} dcode[2];

/* an executable's symbols */
int aout_nsyms;
long *aout_value;
int *aout_class;
char **aout_name;		/* allocated: any length */

/* a name for an address: in an object module, a symbol defined at that
   offset in location counter seg; in an executable (seg -1), a symbol
   with that value, only a text symbol if textonly is set; one whose
   name doesn't start with "__" (such as __btext) is preferred */
char *dlabel(int seg, long addr, int textonly)
{
	char *found = 0;
	int i;

	if (seg < 0) {
		for (i = 0; i < aout_nsyms; i++) {
			int class = aout_class[i];

			if (aout_value[i] != addr || class == 0x10 ||
			    (textonly && class != 0x12 && class != 0x1a))
				continue;
			if (strncmp(aout_name[i], "__", 2) != 0)
				return aout_name[i];
			if (!found)
				found = aout_name[i];
		}
		return found;
	}
	for (i = 0; i < num_syms; i++) {
		unsigned int symtype = symtab[i].symtype & 0x3FFF;

		/* absolute, imported or local common: not a label here */
		if (symtype & (0x10 | 0x40 | 0x2000) || (int)(symtype & 0xF) != seg
		    || symtab[i].offset != addr)
			continue;
		if (strncmp(symnames[i], "__", 2) != 0)
			return symnames[i];
		if (!found)
			found = symnames[i];
	}
	return found;
}

/* the nearest label at or before offset addr in location counter seg,
   as an operand: "label" or "label+$n", or 0 if there is none */
char *dnearest(int seg, long addr)
{
	static char buf[80];
	char *best = 0;
	long bestoff = -1;
	int i;

	for (i = 0; i < num_syms; i++) {
		unsigned int symtype = symtab[i].symtype & 0x3FFF;

		if (symtype & (0x10 | 0x40 | 0x2000) || (int)(symtype & 0xF) != seg
		    || symtab[i].offset > addr || symtab[i].offset < bestoff)
			continue;
		if (symtab[i].offset == bestoff
		    && strncmp(symnames[i], "__", 2) == 0)
			continue;
		best = symnames[i];
		bestoff = symtab[i].offset;
	}
	if (!best)
		return 0;
	if (addr == bestoff)
		return best;
	sprintf(buf, "%.60s+$%lx", best, addr - bestoff);
	return buf;
}

/* start collecting location counters 0 and 1 of an object module */
void dstart(void)
{
	int s;

	for (s = 0; s < 2; s++) {
		dcode[s].len = segsizes[s];
		dcode[s].base = 0;
		dcode[s].seg = s;
		dcode[s].bytes = calloc(segsizes[s] + 4, 1);
		dcode[s].have = calloc(segsizes[s] + 4, 1);
		dcode[s].rel = calloc(segsizes[s] + 4, sizeof(struct dreloc *));
		if (!dcode[s].bytes || !dcode[s].have || !dcode[s].rel) {
			error("Out of memory");
			dcode[s].len = 0;
		}
	}
}

/* a byte of code at offset pos in location counter seg */
void dbyte(int seg, long pos, int ch)
{
	if (seg < 2 && pos >= 0 && pos < dcode[seg].len) {
		dcode[seg].bytes[pos] = ch;
		dcode[seg].have[pos] = 1;
	}
}

/* a relocated field of size bytes at offset pos in counter seg */
void drel(int seg, long pos, int size, char *text)
{
	struct dreloc *r;
	int i;

	if (seg >= 2 || pos < 0 || pos + size > dcode[seg].len)
		return;
	if ((r = malloc(sizeof(*r))) == 0)
		return;
	r->size = size;
	sprintf(r->text, "%.79s", text);
	dcode[seg].rel[pos] = r;
	for (i = 0; i < size; i++)
		dbyte(seg, pos + i, 0);
}

void dline(long addr, unsigned char *bytes, int n, char *label, char *mnem,
	   char *operand)
{
	char hex[16];
	int i;

	hex[0] = 0;
	for (i = 0; i < n && i < 3; i++)
		sprintf(hex + strlen(hex), "%s%02x", i ? " " : "", bytes[i]);
	if (*operand)
		printf("%04lx  %-8s  %-8s %-6s %s\n", addr & 0xFFFF, hex,
		       label ? label : "", mnem, operand);
	else
		printf("%04lx  %-8s  %-8s %s\n", addr & 0xFFFF, hex,
		       label ? label : "", mnem);
}

/* disassemble dc, printing a line for each instruction, or for data
   where there is no instruction */
void disassemble(struct dcode *dc)
{
	long i = 0, n;

	while (i < dc->len) {
		long addr = dc->base + i;
		char *label = dlabel(dc->seg, addr, 1);
		char operand[100], mnem[8], *name;
		struct m6801_op *op;
		struct dreloc *r = 0;
		long value;
		int k, ok;

		if (!dc->have[i]) {	/* skipped: RMB */
			for (n = i; n < dc->len && !dc->have[n]; n++) ;
			sprintf(operand, "%ld", n - i);
			dline(addr, dc->bytes, 0, label, "rmb", operand);
			i = n;
			continue;
		}
		if (dc->rel[i]) {	/* a relocated field that isn't an
					   instruction's operand: data */
			r = dc->rel[i];
			dline(addr, dc->bytes + i, r->size, label,
			      r->size == 1 ? "fcb" : r->size == 2 ? "fdb" :
			      "fqb", r->text);
			i += r->size;
			continue;
		}
		op = &m6801_ops[dc->bytes[i]];
		n = m6801_oplength(op->mode);
		ok = op->mode != M_ILL && i + n <= dc->len;
		for (k = 1; ok && k < n; k++)
			if (!dc->have[i + k]
			    || (dc->rel[i + k] && (k != 1 || dc->rel[i + k]->size
						   != n - 1)))
				ok = 0;
		if (!ok) {		/* not an instruction: data */
			sprintf(operand, "$%02x", dc->bytes[i]);
			dline(addr, dc->bytes + i, 1, label, "fcb", operand);
			i++;
			continue;
		}
		if (n > 1)
			r = dc->rel[i + 1];
		for (k = 0; op->name[k] && k < 7; k++)	/* lower case */
			mnem[k] = op->name[k] >= 'A' && op->name[k] <= 'Z'
			    ? op->name[k] - 'A' + 'a' : op->name[k];
		mnem[k] = 0;
		value = n == 3 ? (dc->bytes[i + 1] << 8) | dc->bytes[i + 2]
		    : n == 2 ? dc->bytes[i + 1] : 0;
		name = 0;
		switch (op->mode) {
		case M_IMM:
			if (r)
				sprintf(operand, "#%s", r->text);
			else
				sprintf(operand, "#$%02lx", value);
			break;
		case M_IMM16:
			if (r)
				sprintf(operand, "#%s", r->text);
			else
				sprintf(operand, "#$%04lx", value);
			break;
		case M_DIR:
			if (r)
				sprintf(operand, "<%s", r->text);
			else if (dc->seg < 0
				 && (name = dlabel(-1, value, 0)) != 0)
				sprintf(operand, "<%.80s", name);
			else
				sprintf(operand, "$%02lx", value);
			break;
		case M_EXT:
			if (r)
				sprintf(operand, "%s", r->text);
			else if (dc->seg < 0
				 && (name = dlabel(-1, value, 0)) != 0)
				sprintf(operand, "%.80s", name);
			else	/* as01 would use direct addressing for
				   an address in page zero */
				sprintf(operand, "%s$%04lx",
					value < 0x100 ? ">" : "", value);
			break;
		case M_IDX:
			if (r)
				sprintf(operand, "%s,x", r->text);
			else
				sprintf(operand, "$%02lx,x", value);
			break;
		case M_REL:
			if (r)
				sprintf(operand, "%s", r->text);
			else {
				value = (addr + 2 + (signed char)value) & 0xFFFF;
				if (value == (addr & 0xFFFF))
					strcpy(operand, "*");	/* itself */
				else if ((name = dlabel(dc->seg, dc->seg < 0 ? value
						   : value - dc->base, 1)) != 0)
					sprintf(operand, "%.80s", name);
				else
					sprintf(operand, "$%04lx", value);
			}
			break;
		default:
			operand[0] = 0;
			break;
		}
		dline(addr, dc->bytes + i, n, label, mnem, operand);
		i += n;
	}
}

/* free a module's collected code */
void dfree(void)
{
	int s;
	long i;

	for (s = 0; s < 2; s++) {
		if (dcode[s].rel)
			for (i = 0; i < dcode[s].len; i++)
				free(dcode[s].rel[i]);
		free(dcode[s].bytes);
		free(dcode[s].have);
		free(dcode[s].rel);
		dcode[s].bytes = 0;
		dcode[s].have = 0;
		dcode[s].rel = 0;
		dcode[s].len = 0;
	}
}

/* read an executable's symbols, for labels */
void load_aout_syms(void)
{
	long n, i;

	free(aout_value);
	free(aout_class);
	if (aout_name)
		for (i = 0; i < aout_nsyms; i++)
			free(aout_name[i]);
	free(aout_name);
	aout_nsyms = 0;
	n = header[7] / 16;
	aout_value = malloc((n + 1) * sizeof(long));
	aout_class = malloc((n + 1) * sizeof(int));
	aout_name = malloc((n + 1) * sizeof(*aout_name));
	if (!aout_value || !aout_class || !aout_name) {
		error("Out of memory");
		return;
	}
	fseek(ifd, h_len + header[2] + header[3] + header[8] + header[9], 0);
	for (i = 0; i < n; i++) {
		char name[8];
		int class;
		long value;

		if (fread(name, 1, 8, ifd) != 8)
			break;
		value = get_long();
		class = getc(ifd);
		getc(ifd);	/* number of auxiliary entries */
		get_word();	/* type */
		if (class == 0) {	/* the rest of the last name */
			char *longer;
			size_t len;

			if (aout_nsyms == 0)
				continue;
			len = strlen(aout_name[aout_nsyms - 1]);
			longer = realloc(aout_name[aout_nsyms - 1], len + 9);
			if (longer == 0)
				continue;
			memcpy(longer + len, name, 8);
			longer[len + 8] = 0;
			aout_name[aout_nsyms - 1] = longer;
			continue;
		}
		if ((aout_name[aout_nsyms] = malloc(9)) == 0)
			break;
		memcpy(aout_name[aout_nsyms], name, 8);
		aout_name[aout_nsyms][8] = 0;
		aout_value[aout_nsyms] = value;
		aout_class[aout_nsyms++] = class;
	}
}

/* disassemble an executable's text segment, from its __btext */
void disasm_aout(void)
{
	struct dcode dc;
	long l;
	int i;

	load_aout_syms();
	dc.base = 0;
	for (i = 0; i < aout_nsyms; i++)
		if (strcmp(aout_name[i], "__btext") == 0)
			dc.base = aout_value[i];
	dc.len = header[2];
	dc.seg = -1;
	dc.bytes = calloc(dc.len + 4, 1);
	dc.have = malloc(dc.len + 4);
	dc.rel = calloc(dc.len + 4, sizeof(struct dreloc *));
	if (!dc.bytes || !dc.have || !dc.rel) {
		error("Out of memory");
		return;
	}
	memset(dc.have, 1, dc.len + 4);
	fseek(ifd, (long)h_len, 0);
	for (l = 0; l < dc.len; l++) {
		if ((i = getc(ifd)) == EOF)
			break;
		dc.bytes[l] = i;
	}
	printf("DISASSEMBLY\n");
	disassemble(&dc);
	printf("\n");
	free(dc.bytes);
	free(dc.have);
	free(dc.rel);
}

/* disassemble an object module's code: location counters 0 and 1 */
void disasm_module(void)
{
	int s;

	if (dcode[0].len == 0 && dcode[1].len == 0)
		return;
	printf("DISASSEMBLY\n");
	for (s = 0; s < 2; s++)
		if (dcode[s].len) {
			printf("SEG %x\n", s);
			disassemble(&dcode[s]);
		}
	printf("\n");
}
