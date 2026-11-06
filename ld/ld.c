/* ld.c - linker for Introl format (6809 C) object files 6809/8086/80386 */

/* Copyright (C) 1994 Bruce Evans */

#include "syshead.h"
#include "const.h"
#include "byteord.h"
#include "type.h"
#include "globvar.h"
#include "script.h"

#define MAX_LIBS	5	/* maximum number of -L directories */

PUBLIC bin_off_t text_base_value = 0;	/* XXX */
PUBLIC bin_off_t data_base_value = 0;	/* XXX */
PUBLIC bin_off_t heap_top_value = 0;	/* XXX */
PUBLIC int headerless = 0;
PUBLIC char hexdigit[] = "0123456789abcdef";

PRIVATE bool_t flag[128];
PRIVATE char *libs[MAX_LIBS];	/* -L directories; there is no default */

PRIVATE int lastlib = 0;

FORWARD char *buildname P((char *pre, char *mid, char *suf));
FORWARD char *expandlib P((char *fn));
FORWARD bin_off_t getaddr P((char *str, char *message));

PRIVATE char *buildname(char *pre, char *mid, char *suf)
{
	char *name;

	name = ourmalloc(strlen(pre) + strlen(mid) + strlen(suf) + 1);
	strcpy(name, pre);
	strcat(name, mid);
	strcat(name, suf);
	return name;
}

/* parse an address like C: 0x (or $) for hex, leading 0 for octal,
   otherwise decimal; anything else, or more than 16 bits, is an error */

PRIVATE bin_off_t getaddr(char *str, char *message)
{
	char *end;
	unsigned long value;
	int base = 0;

	if (*str == '$') {
		++str;
		base = 16;
	}
	errno = 0;
	value = strtoul(str, &end, base);
	if (errno != 0 || end == str || *end != 0 || value > 0xFFFF)
		use_error(message);
	return (bin_off_t) value;
}

PRIVATE char *expandlib(char *fn)
{
	char *path, *s;
	int i;

	for (i = lastlib - 1; i >= 0; --i) {
		path = ourmalloc(strlen(libs[i]) + strlen(fn) + 2);
		strcpy(path, libs[i]);
		s = path + strlen(path);
		if (s != path && s[-1] != '/')
			strcat(path, "/");
		strcat(path, fn);
		if (access(path, R_OK) == 0)
			return path;
		ourfree(path);
	}
	return NUL_PTR;
}

void ld86r(int argc, char **argv);
PUBLIC int main(int argc, char **argv)
{
	register char *arg;
	int argn;
	static char crtprefix[] = "crt";
	static char crtsuffix[] = ".o";
	char *infilename;
	static char libprefix[] = "lib";
	static char libsuffix[] = ".a";
	char *outfilename;
	char *tfn;
	char *scriptfile = NUL_PTR;
	int icount = 0;

	ioinit(argv[0]);
	objinit();
	syminit();
	typeconv_init(INT_BIG_ENDIAN, LONG_BIG_ENDIAN);
	outfilename = NUL_PTR;
	for (argn = 1; argn < argc; ++argn) {
		arg = argv[argn];
		if (*arg != '-') {
			readsyms(arg, flag['t']);
			icount++;
		} else
			switch (arg[1]) {
			case 'v':
				version_msg();
			case 'r':	/* relocatable output */
			case 't':	/* trace modules linked */
				if (icount > 0)
					usage();
			case 'M':	/* print symbols linked */
			case 'P':	/* print the default linker script */
			case 'Z':	/* orphan input sections are errors */
			case 'i':	/* separate I & D output */
			case 'm':	/* print modules linked */
			case 's':	/* strip symbols */
			case 'd':	/* Make a headerless outfile */
			case 'y':	/* Use a newer symbol table */
				if (arg[2] == 0)
					flag[(int)arg[1]] = TRUE;
				else if (arg[2] == '-' && arg[3] == 0)
					flag[(int)arg[1]] = FALSE;
				else
					usage();
				break;
			case 'S':	/* linker script */
				if (arg[2] == 0 && ++argn >= argc)
					usage();
				if (scriptfile != NUL_PTR)
					use_error("only one -S is allowed");
				scriptfile = arg[2] == 0 ? argv[argn] : arg + 2;
				break;
			case 'A':	/* define a symbol */
				if (arg[2] == 0 && ++argn >= argc)
					usage();
				script_define(arg[2] == 0 ? argv[argn] : arg + 2);
				break;
			case 'C':	/* startfile name */
				tfn = buildname(crtprefix, arg + 2, crtsuffix);
				if ((infilename = expandlib(tfn)) == NUL_PTR)
					infilename = tfn;
				/*fatalerror(tfn);  * XXX - need to describe failure */
				readsyms(infilename, flag['t']);
				icount++;
				break;
			case 'L':	/* library path */
				if (lastlib < MAX_LIBS)
					libs[lastlib++] = arg + 2;
				else
					fatalerror("too many library paths");
				break;
			case 'O':	/* library file name */
				if ((infilename =
				     expandlib(arg + 2)) == NUL_PTR)
					infilename = arg + 2;
				/* fatalerror(arg); * XXX */
				readsyms(infilename, flag['t']);
				break;
			case 'T':	/* text base address */
				if (arg[2] == 0 && ++argn >= argc)
					usage();
				text_base_given = 1;
				text_base_value = getaddr(arg[2] == 0 ? argv[argn] : arg + 2,
						"invalid text address");
				break;
			case 'D':	/* data base address */
				if (arg[2] == 0 && ++argn >= argc)
					usage();
				data_base_value = getaddr(arg[2] == 0 ? argv[argn] : arg + 2,
						"invalid data address");
				break;
			case 'H':	/* heap top address */
				if (arg[2] == 0 && ++argn >= argc)
					usage();
				heap_top_value = getaddr(arg[2] == 0 ? argv[argn] : arg + 2,
						"invalid heap size");
				break;
			case 'l':	/* library name */
				tfn = buildname(libprefix, arg + 2, libsuffix);
				if ((infilename = expandlib(tfn)) == NUL_PTR)
					infilename = tfn;
				/* fatalerror(tfn); * XXX */
				readsyms(infilename, flag['t']);
				icount += 2;
				break;
			case 'o':	/* output file name */
				if (arg[2] != 0 || ++argn >= argc
				    || outfilename != NUL_PTR)
					usage();
				outfilename = argv[argn];
				break;
			default:
				usage();
			}
	}
	if (data_base_value && !flag['d'])
		flag['i'] = TRUE;

	if (flag['P']) {
		/* print the default script, as it would be used */
		if (scriptfile != NUL_PTR)
			use_error("-P shows the default script, which -S replaces");
		script_print_default(flag['i'], flag['d']);
		exit(0);
	}
	if (icount == 0)
		usage();
	script_orphan_error = flag['Z'];

#ifdef BUGCOMPAT
	if (icount == 1 && flag['r']) {
		flag['r'] = 0;
	}
#endif

#ifdef REL_OUTPUT
	if (flag['r']) {
		/* Do a relocatable link -- actually fake it with 'ar.c' */
		ld86r(argc, argv);
	}
#endif

	/* Headerless executables can't use symbols. */
	headerless = flag['d'];
	if (headerless)
		flag['s'] = 1;

	if (scriptfile != NUL_PTR)
		script_read(scriptfile);
	else
		script_use_default(flag['i'], headerless);
	if (script_binary) {
		headerless = 1;
		flag['s'] = 1;
	}
	script_set_entry();

	linksyms(flag['r']);
	if (outfilename == NUL_PTR)
		outfilename = "a.out";
	write_elks(outfilename, flag['i'], flag['s'], flag['y']);
	if (flag['m'])
		dumpmods();
	if (flag['M']) {
		script_dump_layout();
		dumpsyms();
	}
	flusherr();
	return errcount ? 1 : 0;
}
