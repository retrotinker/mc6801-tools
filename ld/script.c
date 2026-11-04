/* script.c - linker scripts for ld01
 *
 * A script says which parts of which modules go into each output section,
 * where the sections go, and defines symbols.  Without -S, ld01 uses a
 * built-in default script that reproduces its traditional layout; -P
 * prints it.  See doc/design/ld01-linker-scripts.md.
 *
 * The input sections are the parts of each loaded module's location
 * counters: LOCn for the data part of counter n and COMMONn for its
 * common blocks.
 */

#include "syshead.h"
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "const.h"
#include "obj.h"
#include "type.h"
#include "globvar.h"
#include "script.h"

#define MAXPASSES	10

PUBLIC int script_binary;
PUBLIC int script_orphan_error;
PUBLIC int text_base_given;

PUBLIC bin_off_t lay_text_start;
PUBLIC bin_off_t lay_text_end;
PUBLIC bin_off_t lay_data_start;
PUBLIC bin_off_t lay_data_end;
PUBLIC bin_off_t lay_bss_end;
PUBLIC int lay_sepid;

/* expressions */

enum {
	X_NUM = 1, X_DOT, X_SYM, X_FUNC, X_COND,
	X_NEG, X_COMPL, X_NOT,
	X_MUL, X_DIV, X_MOD, X_ADD, X_SUB, X_SHL, X_SHR,
	X_LT, X_LE, X_GT, X_GE, X_EQ, X_NE,
	X_AND, X_XOR, X_OR, X_LAND, X_LOR
};

struct expr {
	int op;
	long num;
	char *name;			/* symbol, function or its name argument */
	struct expr *a, *b, *c;
	int line;
};

/* statements, sections and symbols */

enum { S_ASSIGN = 1, S_INPUT, S_ONEPAGE, S_SECTION, S_DATA, S_FILL, S_ASSERT };
enum { A_PLAIN = 1, A_PROVIDE, A_HIDDEN };
enum { C_TEXT, C_DATA, C_ABS };		/* symbol classes */

struct piece;

struct stmt {
	int kind;
	int line;
	struct stmt *next;
	/* S_ASSIGN */
	int akind;
	struct ssym *sym;		/* null for "." */
	struct expr *e;
	/* S_INPUT */
	int keep;			/* KEEP(...): load the modules */
	char *filepat;			/* null for orphans */
	char **secpats;
	int nsecpats;
	struct piece **pieces;
	int npieces, maxpieces;
	/* S_ONEPAGE */
	struct stmt *body;
	/* S_SECTION */
	struct osect *sect;
	/* S_DATA: BYTE or WORD; S_FILL; S_ASSERT */
	int size;
	char *message;
};

struct region {
	char *name;
	char *attr;
	int line;
	struct expr *origin, *length;
	long org, len;
	long cursor;			/* next free address */
	struct region *next;
};

/* fill and data that the layout adds to the output */
struct extra {
	struct osect *sect;
	long vma;
	long len;			/* gap length, or 1 or 2 for data */
	long value;			/* fill byte, or data */
	int data;
};

struct osect {
	char *name;
	int line;
	struct expr *addr, *at, *align, *subalign;
	struct stmt *body;
	struct osect *next;
	int datapart;			/* in the a.out data part */
	int noload;			/* NOLOAD: not written */
	struct region *region;		/* > region */
	struct region *lmaregion;	/* AT> region */
	struct expr *fillexpr;		/* = fill */
	long sub;			/* SUBALIGN value, or 1 */
	long vma, lma, size;
	long pvma, plma, psize;		/* previous pass */
};

struct piece {
	struct modstruct *mod;
	int seg;
	int common;
	long size;
	long vma, pvma;
	struct osect *sect;
};

struct ssym {
	char *name;
	struct ssym *next;
	int plain, provide, hidden;	/* kinds of assignment it has */
	int class;			/* from its last assignment */
	int moddef;			/* defined by a loaded module */
	int referenced;			/* imported by a loaded module */
	int defined;			/* defined for the modules and output */
	int assigned;			/* assigned in this pass */
	long value, pvalue;
	int order;			/* order of first assignment */
	struct symstruct *symptr;	/* the import it satisfies */
};

/* the script */

PRIVATE struct stmt *top, **toptail = &top;
PRIVATE struct osect *sects, **secttail = &sects;
PRIVATE struct ssym *ssyms;
PRIVATE int nssyms;
PRIVATE char *entryname;
PRIVATE char *scriptname;
PRIVATE struct stmt *defines, **definestail = &defines;	/* -A */
PRIVATE struct region *regions, **regiontail = &regions;
PRIVATE struct expr *globalfill;	/* top-level FILL */
PRIVATE int includes;			/* depth of INCLUDE */

/* the layout */

PRIVATE struct piece *pieces;
PRIVATE int npieces;
PRIVATE long dot;
PRIVATE int pass;
PRIVATE struct osect *cursect;	/* section being laid out */
PRIVATE struct osect *lastsect;	/* last section laid out */
PRIVATE int sepid_option;
PRIVATE int binary_output;
PRIVATE long curfill;			/* fill byte for gaps here */
PRIVATE struct extra *extras;		/* from the latest pass */
PRIVATE int nextras, maxextras;
PRIVATE struct stmt *failed;		/* first failed ASSERT this pass */
PRIVATE long filelow;			/* lowest load address, for binary */

/* memory and messages */

PRIVATE void *xalloc(size_t n)
{
	void *p = malloc(n ? n : 1);

	if (p == NULL)
		outofmemory();
	memset(p, 0, n ? n : 1);
	return p;
}

PRIVATE char *xstrdup(const char *s)
{
	return strcpy(xalloc(strlen(s) + 1), s);
}

PRIVATE void script_error(int line, const char *format, ...)
{
	char msg[256], buf[400];
	va_list ap;

	va_start(ap, format);
	vsnprintf(msg, sizeof msg, format, ap);
	va_end(ap);
	if (line > 0)
		snprintf(buf, sizeof buf, "%s:%d: %s", scriptname, line, msg);
	else
		snprintf(buf, sizeof buf, "%s: %s", scriptname, msg);
	fatalerror(buf);
}

/* the scanner: works directly on the script text */

PRIVATE char *p;
PRIVATE int line;

PRIVATE int identchar(int c)
{
	return isalnum(c) || c == '_' || c == '.' || c == '$';
}

PRIVATE void skipspace(void)
{
	for (;;) {
		while (isspace((unsigned char)*p)) {
			if (*p == '\n')
				++line;
			++p;
		}
		if (p[0] == '/' && p[1] == '*') {
			p += 2;
			while (*p && !(p[0] == '*' && p[1] == '/')) {
				if (*p == '\n')
					++line;
				++p;
			}
			if (!*p)
				script_error(line, "unterminated comment");
			p += 2;
		} else if (*p == '#') {
			while (*p && *p != '\n')
				++p;
		} else
			return;
	}
}

/* is the next thing the punctuation or keyword s?  if so, skip it */
PRIVATE int accept(const char *s)
{
	size_t n = strlen(s);

	skipspace();
	if (strncmp(p, s, n) != 0)
		return 0;
	if (isalpha((unsigned char)s[0]) && identchar((unsigned char)p[n]))
		return 0;
	p += n;
	return 1;
}

PRIVATE void expect(const char *s)
{
	if (!accept(s))
		script_error(line, "expected \"%s\"", s);
}

/* a name: symbol, section, function or keyword */
PRIVATE char *name(void)
{
	char *start;

	skipspace();
	if (!(isalpha((unsigned char)*p) || *p == '_' || *p == '.'))
		script_error(line, "expected a name");
	start = p;
	while (identchar((unsigned char)*p))
		++p;
	return strncpy(xalloc(p - start + 1), start, p - start);
}

/* a file or section pattern: anything up to punctuation or space */
PRIVATE char *pattern(void)
{
	char *start;

	skipspace();
	start = p;
	while (*p && !isspace((unsigned char)*p) && !strchr("(){};=,", *p))
		++p;
	if (p == start)
		script_error(line, "expected a pattern");
	return strncpy(xalloc(p - start + 1), start, p - start);
}

/* symbols */

PRIVATE struct ssym *findssym(const char *s)
{
	struct ssym *ss;

	for (ss = ssyms; ss != NULL; ss = ss->next)
		if (strcmp(ss->name, s) == 0)
			return ss;
	return NULL;
}

PRIVATE struct ssym *ssym(const char *s)
{
	struct ssym *ss, **tail;

	if ((ss = findssym(s)) != NULL)
		return ss;
	for (tail = &ssyms; *tail != NULL; tail = &(*tail)->next)
		;
	ss = *tail = xalloc(sizeof *ss);
	ss->name = xstrdup(s);
	ss->order = nssyms++;
	return ss;
}

PRIVATE struct osect *findsect(const char *s)
{
	struct osect *sect;

	for (sect = sects; sect != NULL; sect = sect->next)
		if (strcmp(sect->name, s) == 0)
			return sect;
	return NULL;
}

PRIVATE struct region *findregion(const char *s, int line)
{
	struct region *r;

	for (r = regions; r != NULL; r = r->next)
		if (strcmp(r->name, s) == 0)
			return r;
	script_error(line, "there's no memory region %s", s);
	return NULL;
}

/* a string in double quotes */
PRIVATE char *string(void)
{
	char *start;

	skipspace();
	if (*p != '"')
		script_error(line, "expected a string in double quotes");
	start = ++p;
	while (*p && *p != '"' && *p != '\n')
		++p;
	if (*p != '"')
		script_error(line, "unterminated string");
	++p;
	return strncpy(xalloc(p - start), start, p - start - 1);
}

/* expressions */

PRIVATE struct expr *newexpr(int op)
{
	struct expr *e = xalloc(sizeof *e);

	e->op = op;
	e->line = line;
	return e;
}

PRIVATE struct expr *expression(void);

PRIVATE struct expr *number(void)
{
	struct expr *e = newexpr(X_NUM);
	char *end;
	int base = 0;

	if (*p == '$') {
		++p;
		base = 16;
	}
	errno = 0;
	e->num = strtoul(p, &end, base);
	if (end == p || errno != 0)
		script_error(line, "bad number");
	p = end;
	if (*p == 'K' || *p == 'k') {
		e->num *= 1024;
		++p;
	}
	if (identchar((unsigned char)*p))
		script_error(line, "bad number");
	return e;
}

PRIVATE struct expr *primary(void)
{
	struct expr *e;

	skipspace();
	if (isdigit((unsigned char)*p) || *p == '$')
		return number();
	if (accept("(")) {
		e = expression();
		expect(")");
		return e;
	}
	if (*p == '.' && !identchar((unsigned char)p[1])) {
		++p;
		return newexpr(X_DOT);
	}
	e = newexpr(X_SYM);
	e->name = name();
	if (!accept("("))
		return e;
	e->op = X_FUNC;
	if (strcmp(e->name, "ADDR") == 0 || strcmp(e->name, "LOADADDR") == 0
	    || strcmp(e->name, "SIZEOF") == 0
	    || strcmp(e->name, "DEFINED") == 0) {
		e->a = newexpr(X_SYM);
		e->a->name = name();
	} else if (strcmp(e->name, "ALIGN") == 0) {
		e->a = expression();
		if (accept(","))
			e->b = expression();
	} else if (strcmp(e->name, "MAX") == 0 || strcmp(e->name, "MIN") == 0) {
		e->a = expression();
		expect(",");
		e->b = expression();
	} else if (strcmp(e->name, "ORIGIN") == 0
		   || strcmp(e->name, "LENGTH") == 0) {
		e->a = newexpr(X_SYM);
		e->a->name = name();
	} else
		script_error(line, "unknown function %s()", e->name);
	expect(")");
	return e;
}

PRIVATE struct expr *unary(void)
{
	struct expr *e;

	if (accept("-")) {
		e = newexpr(X_NEG);
		e->a = unary();
		return e;
	}
	if (accept("~")) {
		e = newexpr(X_COMPL);
		e->a = unary();
		return e;
	}
	if (accept("!")) {
		e = newexpr(X_NOT);
		e->a = unary();
		return e;
	}
	if (accept("+"))
		return unary();
	return primary();
}

/* binary operators, by precedence level, lowest first */
PRIVATE struct {
	const char *s;
	int op;
	int level;
} binops[] = {
	{ "||", X_LOR, 1 }, { "&&", X_LAND, 2 }, { "|", X_OR, 3 },
	{ "^", X_XOR, 4 }, { "&", X_AND, 5 },
	{ "==", X_EQ, 6 }, { "!=", X_NE, 6 },
	{ "<=", X_LE, 7 }, { ">=", X_GE, 7 },
	{ "<<", X_SHL, 8 }, { ">>", X_SHR, 8 },
	{ "<", X_LT, 7 }, { ">", X_GT, 7 },
	{ "+", X_ADD, 9 }, { "-", X_SUB, 9 },
	{ "*", X_MUL, 10 }, { "/", X_DIV, 10 }, { "%", X_MOD, 10 },
	{ NULL, 0, 0 }
};

PRIVATE struct expr *binary(int level)
{
	struct expr *e, *b;
	int i;

	if (level > 10)
		return unary();
	e = binary(level + 1);
	for (;;) {
		skipspace();
		for (i = 0; binops[i].s != NULL; ++i) {
			size_t n = strlen(binops[i].s);

			if (binops[i].level != level
			    || strncmp(p, binops[i].s, n) != 0)
				continue;
			/* don't take "|" from "||", "<" from "<<" etc. */
			if (n == 1 && (p[1] == p[0] || p[1] == '=')
			    && strchr("|&<>", p[0]))
				continue;
			if (n == 1 && p[0] == '!')
				continue;
			break;
		}
		if (binops[i].s == NULL)
			return e;
		p += strlen(binops[i].s);
		b = newexpr(binops[i].op);
		b->a = e;
		b->b = binary(level + 1);
		e = b;
	}
}

PRIVATE struct expr *expression(void)
{
	struct expr *e = binary(1), *c;

	if (!accept("?"))
		return e;
	c = newexpr(X_COND);
	c->a = e;
	c->b = expression();
	expect(":");
	c->c = expression();
	return c;
}

/* does an expression depend on addresses or symbols? */
PRIVATE int relative(struct expr *e)
{
	if (e == NULL)
		return 0;
	switch (e->op) {
	case X_NUM:
		return 0;
	case X_DOT:
	case X_SYM:
		return 1;
	case X_FUNC:
		return strcmp(e->name, "MAX") != 0 && strcmp(e->name, "MIN") != 0
		    ? 1 : relative(e->a) || relative(e->b);
	default:
		return relative(e->a) || relative(e->b) || relative(e->c);
	}
}

/* statements */

PRIVATE struct stmt *newstmt(int kind)
{
	struct stmt *s = xalloc(sizeof *s);

	s->kind = kind;
	s->line = line;
	return s;
}

/* name = expr ;  PROVIDE(name = expr) ;  HIDDEN(name = expr) ; */
PRIVATE struct stmt *assignment(int akind, char *target)
{
	struct stmt *s = newstmt(S_ASSIGN);

	s->akind = akind;
	if (akind != A_PLAIN) {
		expect("(");
		target = name();
	}
	if (strcmp(target, ".") != 0)
		s->sym = ssym(target);
	else if (akind != A_PLAIN)
		script_error(line, "can't use \".\" in PROVIDE or HIDDEN");
	expect("=");
	s->e = expression();
	if (akind != A_PLAIN) {
		expect(")");
		accept(";");
	} else
		expect(";");
	if (s->sym != NULL) {
		if (akind == A_PLAIN)
			s->sym->plain = 1;
		else if (akind == A_PROVIDE)
			s->sym->provide = 1;
		else
			s->sym->hidden = 1;
	}
	return s;
}

PRIVATE struct stmt *statements(void);
PRIVATE struct stmt *input(char *word);

/* ASSERT(expression, "message") */
PRIVATE struct stmt *assertion(void)
{
	struct stmt *s = newstmt(S_ASSERT);

	expect("(");
	s->e = expression();
	expect(",");
	s->message = string();
	expect(")");
	accept(";");
	return s;
}

/* one statement inside a section */
PRIVATE struct stmt *statement(void)
{
	struct stmt *s;

	if (accept("PROVIDE"))
		return assignment(A_PROVIDE, NULL);
	if (accept("HIDDEN"))
		return assignment(A_HIDDEN, NULL);
	if (accept("ONEPAGE")) {
		s = newstmt(S_ONEPAGE);
		expect("{");
		s->body = statements();
		expect("}");
		return s;
	}
	if (accept("BYTE") || accept("WORD")) {
		s = newstmt(S_DATA);
		s->size = p[-1] == 'E' ? 1 : 2;		/* BYTE or WORD */
		expect("(");
		s->e = expression();
		expect(")");
		accept(";");
		return s;
	}
	if (accept("FILL")) {
		s = newstmt(S_FILL);
		expect("(");
		s->e = expression();
		expect(")");
		accept(";");
		return s;
	}
	if (accept("ASSERT"))
		return assertion();
	if (accept("KEEP")) {
		expect("(");
		s = input(pattern());
		expect(")");
		s->keep = 1;
		return s;
	}
	return input(pattern());
}

/* file(sections), or an assignment to a name read as a pattern */
PRIVATE struct stmt *input(char *word)
{
	struct stmt *s;

	skipspace();
	if (*p == '=')
		return assignment(A_PLAIN, word);
	if (*p != '(')
		script_error(line, "expected an input pattern or an assignment");
	++p;
	s = newstmt(S_INPUT);
	s->filepat = word;
	while (!accept(")")) {
		s->secpats = realloc(s->secpats,
				     (s->nsecpats + 1) * sizeof *s->secpats);
		if (s->secpats == NULL)
			outofmemory();
		s->secpats[s->nsecpats++] = pattern();
	}
	if (s->nsecpats == 0)
		script_error(s->line, "no input sections in %s()", word);
	return s;
}

PRIVATE struct stmt *statements(void)
{
	struct stmt *first = NULL, **tail = &first;

	skipspace();
	while (*p && *p != '}') {
		*tail = statement();
		tail = &(*tail)->next;
		skipspace();
	}
	return first;
}

/* is the next thing "(NOLOAD)"?  if so, skip it */
PRIVATE int noload(void)
{
	char *save = p;

	if (accept("(") && accept("NOLOAD") && accept(")"))
		return 1;
	p = save;
	return 0;
}

/* name [address] [(NOLOAD)] : [AT(lma)] [ALIGN(n)] [SUBALIGN(n)]
	{ statements } [> region] [AT> region] [= fill] */
PRIVATE struct stmt *section(char *sname)
{
	struct stmt *s = newstmt(S_SECTION);
	struct osect *sect = xalloc(sizeof *sect);

	if (findsect(sname) != NULL)
		script_error(line, "section %s is defined more than once", sname);
	sect->name = sname;
	sect->line = line;
	skipspace();
	if (noload())
		sect->noload = 1;
	else if (*p != ':') {
		sect->addr = expression();
		sect->noload = noload();
	}
	expect(":");
	for (;;) {
		if (accept("AT")) {
			expect("(");
			sect->at = expression();
			expect(")");
		} else if (accept("ALIGN")) {
			expect("(");
			sect->align = expression();
			expect(")");
		} else if (accept("SUBALIGN")) {
			expect("(");
			sect->subalign = expression();
			expect(")");
		} else
			break;
	}
	expect("{");
	sect->body = statements();
	expect("}");
	for (;;) {
		if (accept("AT")) {
			expect(">");
			sect->lmaregion = findregion(name(), line);
		} else if (accept(">"))
			sect->region = findregion(name(), line);
		else if (accept("=") ) {
			sect->fillexpr = expression();
		} else
			break;
	}
	s->sect = sect;
	*secttail = sect;
	secttail = &sect->next;
	return s;
}

PRIVATE void addtop(struct stmt *s)
{
	*toptail = s;
	toptail = &s->next;
}

PRIVATE void commands(void);
PRIVATE void sections_body(int toplevel);

PRIVATE char *readfile(char *filename)
{
	FILE *f;
	char *text;
	long n;

	if ((f = fopen(filename, "r")) == NULL) {
		scriptname = filename;
		script_error(0, "can't open the linker script");
	}
	fseek(f, 0, SEEK_END);
	n = ftell(f);
	rewind(f);
	text = xalloc(n + 1);
	if (n < 0 || fread(text, 1, n, f) != (size_t)n) {
		scriptname = filename;
		script_error(0, "can't read the linker script");
	}
	text[n] = '\0';
	fclose(f);
	return text;
}

/* INCLUDE "file": read it in the current context */
PRIVATE void include(int insections)
{
	char *filename = string();
	char *savep = p, *savename = scriptname;
	int saveline = line;

	if (++includes > 10)
		script_error(line, "INCLUDE is nested too deeply");
	p = readfile(filename);
	scriptname = filename;
	line = 1;
	if (insections)
		sections_body(1);
	else
		commands();
	p = savep;
	scriptname = savename;
	line = saveline;
	--includes;
}

/* MEMORY { name [(attributes)] : ORIGIN = expr, LENGTH = expr ... } */
PRIVATE void memory(void)
{
	struct region *r;
	char *start;

	expect("{");
	while (!accept("}")) {
		r = xalloc(sizeof *r);
		r->line = line;
		r->name = name();
		if (accept("(")) {
			skipspace();
			start = p;
			while (*p && *p != ')')
				++p;
			r->attr = strncpy(xalloc(p - start + 1), start, p - start);
			expect(")");
		}
		expect(":");
		if (!accept("ORIGIN") && !accept("org") && !accept("o"))
			script_error(line, "expected ORIGIN");
		expect("=");
		r->origin = expression();
		accept(",");
		if (!accept("LENGTH") && !accept("len") && !accept("l"))
			script_error(line, "expected LENGTH");
		expect("=");
		r->length = expression();
		accept(",");
		*regiontail = r;
		regiontail = &r->next;
	}
}

/* the contents of SECTIONS, up to "}" (or the end of an included file) */
PRIVATE void sections_body(int infile)
{
	char *word;

	for (;;) {
		skipspace();
		if (!*p) {
			if (infile)
				return;
			script_error(line, "expected \"}\"");
		}
		if (!infile && accept("}"))
			return;
		if (accept("PROVIDE"))
			addtop(assignment(A_PROVIDE, NULL));
		else if (accept("HIDDEN"))
			addtop(assignment(A_HIDDEN, NULL));
		else if (accept("ASSERT"))
			addtop(assertion());
		else if (accept("INCLUDE"))
			include(1);
		else {
			word = name();
			skipspace();
			if (*p == '=' && p[1] != '=')
				addtop(assignment(A_PLAIN, word));
			else
				addtop(section(word));
		}
	}
}

/* top-level commands, up to the end of the text */
PRIVATE void commands(void)
{
	char *word;

	for (;;) {
		skipspace();
		if (!*p)
			break;
		if (accept("ENTRY")) {
			expect("(");
			entryname = name();
			expect(")");
		} else if (accept("OUTPUT_FORMAT")) {
			expect("(");
			word = name();
			if (strcmp(word, "binary") == 0)
				script_binary = 1;
			else if (strcmp(word, "aout") != 0)
				script_error(line, "unknown output format %s", word);
			expect(")");
		} else if (accept("SECTIONS")) {
			expect("{");
			sections_body(0);
		} else if (accept("MEMORY"))
			memory();
		else if (accept("INCLUDE"))
			include(0);
		else if (accept("FILL")) {
			expect("(");
			globalfill = expression();
			expect(")");
		} else if (accept("ASSERT"))
			addtop(assertion());
		else if (accept("PROVIDE"))
			addtop(assignment(A_PROVIDE, NULL));
		else if (accept("HIDDEN"))
			addtop(assignment(A_HIDDEN, NULL));
		else {
			word = name();
			skipspace();
			if (*p != '=')
				script_error(line, "unknown command %s", word);
			addtop(assignment(A_PLAIN, word));
		}
	}
}

PRIVATE void parse(char *text, char *filename)
{
	scriptname = filename;
	p = text;
	line = 1;
	commands();
}

/* the default script */

PRIVATE char *buf;
PRIVATE size_t buflen, bufsize;

PRIVATE void add(const char *format, ...)
{
	va_list ap;
	int n;

	for (;;) {
		va_start(ap, format);
		n = vsnprintf(buf + buflen, bufsize - buflen, format, ap);
		va_end(ap);
		if (n >= 0 && buflen + n < bufsize)
			break;
		bufsize = bufsize ? 2 * bufsize : 4096;
		buf = realloc(buf, bufsize);
		if (buf == NULL)
			outofmemory();
	}
	buflen += n;
}

/* the PROVIDEs and input statements for one location counter */
PRIVATE void addcounter(int seg, int indent, int data)
{
	const char *in = indent ? "            " : "        ";

	add("%sPROVIDE(__seg%XDL = .);\n", in, seg);
	if (data)
		add("%sHIDDEN(__dpos = .);\n", in);
	add("%s*(LOC%d)\n", in, seg);
	if (data)
		add("%sHIDDEN(__dend = . != __dpos ? . : __dend);\n", in);
	add("%sPROVIDE(__seg%XDH = .);\n", in, seg);
	add("%sPROVIDE(__seg%XCL = .);\n", in, seg);
	add("%s*(COMMON%d)\n", in, seg);
	add("%sPROVIDE(__seg%XCH = .);\n", in, seg);
}

PRIVATE char *default_script(int sepid, int binary)
{
	char dataaddr[24], dvalue[24], hvalue[24];
	int seg;

	buflen = 0;
	if (data_base_value)
		snprintf(dataaddr, sizeof dataaddr, "0x%04lX",
			 (unsigned long)data_base_value);
	else if (sepid)
		strcpy(dataaddr, "0x0000");
	else
		strcpy(dataaddr, "__etext");
	if (data_base_value)
		snprintf(dvalue, sizeof dvalue, "-D 0x%04lX",
			 (unsigned long)data_base_value);
	else
		strcpy(dvalue, "no -D");
	if (heap_top_value)
		snprintf(hvalue, sizeof hvalue, "-H 0x%04lX",
			 (unsigned long)heap_top_value);
	else
		strcpy(hvalue, "no -H");

	add("/* Default linker script for ld01.\n");
	add("   Values from the command line: -T 0x%04lX, %s, %s. */\n\n",
	    (unsigned long)text_base_value, dvalue, hvalue);
	add("OUTPUT_FORMAT(%s)\n\n", binary ? "binary" : "aout");
	add("SECTIONS\n{\n");
	add("    .text 0x%04lX : SUBALIGN(4)\n    {\n",
	    (unsigned long)text_base_value);
	add("        __btext = .;\n");
	add("        PROVIDE(__segoff = 0);\n");
	addcounter(0, 0, 0);
	addcounter(1, 0, 0);
	add("        __etext = .;\n");
	if (sepid)
		add("        . = ALIGN(16);\n");
	add("    }\n\n");
	add("    .data %s : SUBALIGN(4)\n    {\n", dataaddr);
	add("        __bdata = .;\n");
	add("        HIDDEN(__dend = .);\n\n");
	add("        ONEPAGE\n        {\n");
	addcounter(2, 1, 1);
	add("        }\n");
	for (seg = 3; seg < NSEG; ++seg) {
		add("\n");
		addcounter(seg, 0, 1);
	}
	add("\n        __edata = __dend;\n");
	add("        __end = .;\n");
	add("    }\n\n");
	add("    PROVIDE(__heap_top = __end + 0x%04lX);\n",
	    (unsigned long)heap_top_value);
	add("}\n");
	return buf;
}

PUBLIC void script_print_default(sepid, binary)
int sepid;
int binary;
{
	fputs(default_script(sepid, binary), stdout);
}

PUBLIC void script_use_default(sepid, binary)
int sepid;
int binary;
{
	parse(xstrdup(default_script(sepid, binary)), "<default script>");
}

PUBLIC void script_read(filename)
char *filename;
{
	parse(readfile(filename), filename);
}

/* -A sym=value: a plain assignment, before the script's own statements */
PUBLIC void script_define(arg)
char *arg;
{
	char *text = xalloc(strlen(arg) + 2);
	struct stmt *s;
	char *eq;

	scriptname = "-A";
	if ((eq = strchr(arg, '=')) == NULL || eq == arg)
		script_error(0, "expected symbol=value, not \"%s\"", arg);
	sprintf(text, "%s;", arg);
	p = text;
	line = 0;
	s = assignment(A_PLAIN, name());
	skipspace();
	if (*p)
		script_error(0, "expected symbol=value, not \"%s\"", arg);
	*definestail = s;
	definestail = &s->next;
}

/* the entry point */

PUBLIC void script_set_entry()
{
	struct symstruct *symptr;
	struct entrylist *elptr;

	if (defines != NULL) {
		/* -A assignments go first */
		*definestail = top;
		top = defines;
	}
	if (entryname == NULL)
		return;
	if ((symptr = findsym(entryname)) == NULL || (symptr->flags & I_MASK))
		script_error(0, "entry symbol %s isn't defined by a module",
			     entryname);
	/* the script's ENTRY comes before any in the object files */
	elptr = (struct entrylist *)ourmalloc(sizeof(struct entrylist));
	elptr->elsymptr = symptr;
	elptr->elnext = entryfirst;
	entryfirst = elptr;
}

/* pattern matching */

PRIVATE int glob(const char *pat, const char *s)
{
	for (; *pat; ++pat, ++s) {
		if (*pat == '*') {
			for (;; ++s) {
				if (glob(pat + 1, s))
					return 1;
				if (!*s)
					return 0;
			}
		}
		if (!*s || (*pat != '?' && *pat != *s))
			return 0;
	}
	return !*s;
}

PRIVATE const char *basename(const char *s)
{
	const char *slash = strrchr(s, '/');

	return slash ? slash + 1 : s;
}

PRIVATE int filematch(const char *pat, struct modstruct *mod)
{
	char name[512];

	if (mod->archentry == NULL)
		return glob(pat, mod->filename)
		    || glob(pat, basename(mod->filename));
	snprintf(name, sizeof name, "%s:%s", mod->filename, mod->archentry);
	if (glob(pat, name))
		return 1;
	snprintf(name, sizeof name, "%s:%s", basename(mod->filename),
		 mod->archentry);
	return glob(pat, name);
}

PRIVATE int secmatch(const char *pat, int seg, int common)
{
	char name[16];

	if (common) {
		if (strcmp(pat, "COMMON") == 0)
			return 1;
		snprintf(name, sizeof name, "COMMON%d", seg);
	} else {
		if ((strcmp(pat, ".text") == 0 && seg == 0)
		    || (strcmp(pat, ".data") == 0 && seg == 3))
			return 1;
		snprintf(name, sizeof name, "LOC%d", seg);
	}
	return glob(pat, name);
}

/* can a statement (or any inside it) take input from data counters? */
PRIVATE int takesdata(struct stmt *s)
{
	int i, seg;

	for (; s != NULL; s = s->next) {
		if (s->kind == S_ONEPAGE && takesdata(s->body))
			return 1;
		if (s->kind != S_INPUT)
			continue;
		for (i = 0; i < s->nsecpats; ++i)
			for (seg = 2; seg < NSEG; ++seg)
				if (secmatch(s->secpats[i], seg, 0)
				    || secmatch(s->secpats[i], seg, 1))
					return 1;
	}
	return 0;
}

PRIVATE void addpiece(struct stmt *s, struct piece *pc, struct osect *sect)
{
	if (s->npieces == s->maxpieces) {
		s->maxpieces = s->maxpieces ? 2 * s->maxpieces : 16;
		s->pieces = realloc(s->pieces, s->maxpieces * sizeof *s->pieces);
		if (s->pieces == NULL)
			outofmemory();
	}
	s->pieces[s->npieces++] = pc;
	pc->sect = sect;
}

/* give each input section to the first statement that matches it */
PRIVATE void assign(struct stmt *s, struct osect *sect)
{
	int i, j;

	for (; s != NULL; s = s->next) {
		if (s->kind == S_ONEPAGE)
			assign(s->body, sect);
		if (s->kind != S_INPUT)
			continue;
		for (i = 0; i < npieces; ++i) {
			struct piece *pc = &pieces[i];

			if (pc->sect != NULL || !filematch(s->filepat, pc->mod))
				continue;
			for (j = 0; j < s->nsecpats; ++j)
				if (secmatch(s->secpats[j], pc->seg, pc->common)) {
					addpiece(s, pc, sect);
					break;
				}
		}
	}
}

PRIVATE char *piecename(struct piece *pc)
{
	static char name[600];

	snprintf(name, sizeof name, "%s%s%s(%s%d)", pc->mod->filename,
		 pc->mod->archentry ? ":" : "",
		 pc->mod->archentry ? pc->mod->archentry : "",
		 pc->common ? "COMMON" : "LOC", pc->seg);
	return name;
}

/* input sections that no statement matched go after the last section of
   the same kind, as in the default script */
PRIVATE void orphans(void)
{
	struct osect *sect, *last[2];
	struct stmt *orphan[2], **tail;
	char msg[700];
	int i, kind;

	last[0] = last[1] = NULL;
	orphan[0] = orphan[1] = NULL;
	for (sect = sects; sect != NULL; sect = sect->next)
		if (!sect->noload)
			last[sect->datapart] = sect;
	for (sect = sects; sect != NULL; sect = sect->next)
		if (last[sect->datapart] == NULL)
			last[sect->datapart] = sect;
	if (last[1] == NULL)
		last[1] = last[0];
	if (last[0] == NULL)
		last[0] = last[1];
	for (i = 0; i < npieces; ++i) {
		struct piece *pc = &pieces[i];

		if (pc->sect != NULL)
			continue;
		kind = pc->seg >= 2;
		if (last[kind] == NULL)
			script_error(0, "the script has no output sections for %s",
				     piecename(pc));
		if (pc->size != 0) {
			snprintf(msg, sizeof msg,
				 "%s isn't placed by the script%s%s",
				 piecename(pc),
				 script_orphan_error ? "" : "; adding it to ",
				 script_orphan_error ? "" : last[kind]->name);
			if (script_orphan_error)
				script_error(0, "%s", msg);
			warning(msg);
		}
		if (orphan[kind] == NULL) {
			orphan[kind] = newstmt(S_INPUT);
			for (tail = &last[kind]->body; *tail != NULL;
			     tail = &(*tail)->next)
				;
			*tail = orphan[kind];
		}
		addpiece(orphan[kind], pc, last[kind]);
	}
}

/* evaluation */

PRIVATE long eval(struct expr *e);

/* the address of a symbol defined by a module, as currently placed */
PRIVATE long modvalue(struct symstruct *symptr)
{
	int seg = symptr->flags & SEGM_MASK;

	if (symptr->flags & A_MASK)
		return symptr->value;
	if (symptr->flags & (C_MASK | SA_MASK))
		return symptr->modptr->comaddr[seg] + symptr->value;
	return symptr->modptr->segaddr[seg] + symptr->value;
}

/* defined by a loaded module: exported, or a common block (imported
   with a size, before ld01 allocates it) */
PRIVATE int moddefined(struct symstruct *symptr)
{
	return symptr != NULL && symptr->modptr != NULL
	    && symptr->modptr->loadflag
	    && (!(symptr->flags & I_MASK)
		|| (symptr->flags & (C_MASK | SA_MASK))
		|| symptr->value != 0);
}

PRIVATE long symvalue(struct expr *e)
{
	struct ssym *ss = findssym(e->name);
	struct symstruct *symptr;

	if (ss != NULL && (ss->plain || ss->hidden
			   || (ss->provide && !ss->moddef))) {
		if (ss->assigned)
			return ss->value;
		return ss->pvalue;
	}
	symptr = findsym(e->name);
	if (moddefined(symptr) && !(ss != NULL && ss->symptr == symptr))
		return modvalue(symptr);
	script_error(e->line, "symbol %s isn't defined", e->name);
	return 0;
}

PRIVATE struct osect *sectarg(struct expr *e)
{
	struct osect *sect = findsect(e->a->name);

	if (sect == NULL)
		script_error(e->line, "there's no section %s", e->a->name);
	return sect;
}

PRIVATE long align(long value, long n, int line)
{
	if (n <= 0 || (n & (n - 1)) != 0)
		script_error(line, "alignment %ld isn't a power of 2", n);
	return (value + n - 1) & ~(n - 1);
}

PRIVATE long func(struct expr *e)
{
	struct ssym *ss;
	struct osect *sect;
	long a, b;

	/* sections not yet laid out in this pass have the values from the
	   previous one */
	if (strcmp(e->name, "ADDR") == 0)
		return sectarg(e)->vma;
	if (strcmp(e->name, "LOADADDR") == 0)
		return sectarg(e)->lma;
	if (strcmp(e->name, "SIZEOF") == 0) {
		sect = sectarg(e);
		return sect == cursect ? dot - sect->vma : sect->size;
	}
	if (strcmp(e->name, "DEFINED") == 0) {
		ss = findssym(e->a->name);
		return (ss != NULL && ss->assigned)
		    || moddefined(findsym(e->a->name));
	}
	if (strcmp(e->name, "ORIGIN") == 0)
		return findregion(e->a->name, e->line)->org;
	if (strcmp(e->name, "LENGTH") == 0)
		return findregion(e->a->name, e->line)->len;
	if (strcmp(e->name, "ALIGN") == 0) {
		if (e->b == NULL)
			return align(dot, eval(e->a), e->line);
		return align(eval(e->a), eval(e->b), e->line);
	}
	a = eval(e->a);
	b = eval(e->b);
	if (strcmp(e->name, "MAX") == 0)
		return a > b ? a : b;
	return a < b ? a : b;
}

PRIVATE long eval(struct expr *e)
{
	long a, b;

	switch (e->op) {
	case X_NUM:
		return e->num;
	case X_DOT:
		return dot;
	case X_SYM:
		return symvalue(e);
	case X_FUNC:
		return func(e);
	case X_COND:
		return eval(e->a) ? eval(e->b) : eval(e->c);
	case X_NEG:
		return -eval(e->a);
	case X_COMPL:
		return ~eval(e->a) & 0xFFFF;
	case X_NOT:
		return !eval(e->a);
	case X_LAND:
		return eval(e->a) && eval(e->b);
	case X_LOR:
		return eval(e->a) || eval(e->b);
	}
	a = eval(e->a);
	b = eval(e->b);
	switch (e->op) {
	case X_MUL:
		return a * b;
	case X_DIV:
	case X_MOD:
		if (b == 0)
			script_error(e->line, "division by zero");
		return e->op == X_DIV ? a / b : a % b;
	case X_ADD:
		return a + b;
	case X_SUB:
		return a - b;
	case X_SHL:
		return a << b;
	case X_SHR:
		return a >> b;
	case X_LT:
		return a < b;
	case X_LE:
		return a <= b;
	case X_GT:
		return a > b;
	case X_GE:
		return a >= b;
	case X_EQ:
		return a == b;
	case X_NE:
		return a != b;
	case X_AND:
		return a & b;
	case X_XOR:
		return a ^ b;
	default:
		return a | b;
	}
}

/* layout */

PRIVATE void walk(struct stmt *s, long subalign);

PRIVATE void addextra(long vma, long len, long value, int data)
{
	struct extra *x;

	if (nextras == maxextras) {
		maxextras = maxextras ? 2 * maxextras : 32;
		extras = realloc(extras, maxextras * sizeof *extras);
		if (extras == NULL)
			outofmemory();
	}
	x = &extras[nextras++];
	x->sect = cursect;
	x->vma = vma;
	x->len = len;
	x->value = value;
	x->data = data;
}

/* a gap from here to address "to", filled with the current fill byte */
PRIVATE void gap(long to)
{
	if (to > dot && curfill != 0 && cursect != NULL)
		addextra(dot, to - dot, curfill, 0);
}

PRIVATE long fillbyte(struct expr *e)
{
	long value = eval(e);

	if (value < -128 || value > 255)
		script_error(e->line, "fill value %ld isn't a byte", value);
	return value & 0xFF;
}

PRIVATE void assign_symbol(struct stmt *s)
{
	struct ssym *ss = s->sym;
	long value;

	if (s->akind == A_PROVIDE && (ss->plain || ss->moddef))
		return;
	value = eval(s->e);
	ss->value = value;
	ss->assigned = 1;
}

PRIVATE void walk(struct stmt *s, long subalign)
{
	long start, size, value;
	int i;

	for (; s != NULL; s = s->next) {
		switch (s->kind) {
		case S_ASSIGN:
			if (s->sym != NULL) {
				assign_symbol(s);
				break;
			}
			value = eval(s->e);
			if (value < dot)
				script_error(s->line,
					     "\".\" can't move backwards in a section");
			gap(value);
			dot = value;
			break;
		case S_DATA:
			value = eval(s->e);
			if (s->size == 1 ? value < -128 || value > 255
			    : value < -32768 || value > 65535)
				script_error(s->line, "%s value %ld is too large",
					     s->size == 1 ? "BYTE" : "WORD", value);
			addextra(dot, s->size, value, 1);
			dot += s->size;
			break;
		case S_FILL:
			curfill = fillbyte(s->e);
			break;
		case S_ASSERT:
			if (!eval(s->e) && failed == NULL)
				failed = s;
			break;
		case S_INPUT:
			for (i = 0; i < s->npieces; ++i) {
				struct piece *pc = s->pieces[i];

				long pad = (subalign - pc->size % subalign)
				    % subalign;

				pc->vma = dot;
				if (pc->common)
					pc->mod->comaddr[pc->seg] = dot;
				else {
					pc->mod->segaddr[pc->seg] = dot;
					pc->mod->segpad[pc->seg] = pad;
					pc->mod->segfill[pc->seg] = curfill;
					pc->mod->segnoload[pc->seg] =
					    pc->sect->noload;
				}
				dot += pc->size + pad;
			}
			break;
		case S_ONEPAGE: {
			int saveextras = nextras;
			long savefill = curfill;

			start = dot;
			walk(s->body, subalign);
			size = dot - start;
			if (size > 0x100)
				script_error(s->line,
					     "ONEPAGE contents are larger than a page");
			if (((start + size) ^ start) & ~0xFFL) {
				/* move to the next page and lay it out again */
				nextras = saveextras;
				curfill = savefill;
				dot = start;
				gap((start + 0xFF) & ~0xFFL);
				dot = (start + 0xFF) & ~0xFFL;
				walk(s->body, subalign);
			}
			break;
		}
		}
	}
}

PRIVATE void layout_section(struct osect *sect)
{
	long start, subalign = 1;

	cursect = sect;
	if (sect->addr != NULL)
		start = eval(sect->addr);
	else if (sect->region != NULL)
		start = sect->region->cursor;
	else
		start = dot;
	if (text_base_given && strcmp(sect->name, ".text") == 0)
		start = text_base_value;
	if (data_base_value && strcmp(sect->name, ".data") == 0)
		start = data_base_value;
	if (sect->align != NULL)
		start = align(start, eval(sect->align), sect->line);
	if (sect->subalign != NULL)
		subalign = align(1, eval(sect->subalign), sect->line);
	sect->sub = subalign;
	sect->vma = start;
	if (sect->at != NULL)
		sect->lma = eval(sect->at);
	else if (sect->lmaregion != NULL)
		sect->lma = sect->lmaregion->cursor;
	else
		sect->lma = start;
	curfill = sect->fillexpr != NULL ? fillbyte(sect->fillexpr)
	    : globalfill != NULL ? fillbyte(globalfill) : 0;
	dot = start;
	walk(sect->body, subalign);
	sect->size = dot - start;
	if (sect->region != NULL && start + sect->size > sect->region->cursor)
		sect->region->cursor = start + sect->size;
	if (sect->lmaregion != NULL
	    && sect->lma + sect->size > sect->lmaregion->cursor)
		sect->lmaregion->cursor = sect->lma + sect->size;
	cursect = NULL;
	lastsect = sect;
}

PRIVATE void layout_pass(void)
{
	struct stmt *s;
	struct ssym *ss;

	struct region *r;

	for (ss = ssyms; ss != NULL; ss = ss->next)
		ss->assigned = 0;
	nextras = 0;
	failed = NULL;
	dot = 0;
	for (r = regions; r != NULL; r = r->next) {
		r->org = eval(r->origin);
		r->len = eval(r->length);
		r->cursor = r->org;
	}
	lastsect = NULL;
	for (s = top; s != NULL; s = s->next)
		if (s->kind == S_SECTION)
			layout_section(s->sect);
		else if (s->kind == S_ASSERT) {
			if (!eval(s->e) && failed == NULL)
				failed = s;
		} else if (s->sym != NULL)
			assign_symbol(s);
		else
			dot = eval(s->e);
}

/* did anything change since the previous pass?  then remember it */
PRIVATE int remember(void)
{
	struct osect *sect;
	struct ssym *ss;
	int i, diff = 0;

	for (sect = sects; sect != NULL; sect = sect->next) {
		diff |= sect->vma != sect->pvma || sect->lma != sect->plma
		    || sect->size != sect->psize;
		sect->pvma = sect->vma;
		sect->plma = sect->lma;
		sect->psize = sect->size;
	}
	for (ss = ssyms; ss != NULL; ss = ss->next) {
		diff |= ss->value != ss->pvalue;
		ss->pvalue = ss->value;
	}
	for (i = 0; i < npieces; ++i) {
		diff |= pieces[i].vma != pieces[i].pvma;
		pieces[i].pvma = pieces[i].vma;
	}
	return diff;
}

/* which script symbols are defined, and the imports they satisfy */
PUBLIC void script_mark_imports()
{
	struct modstruct *modptr;
	struct symstruct **symparray;
	struct symstruct *symptr;
	struct stmt *s;
	struct osect *sect;
	struct ssym *ss;
	int class = C_ABS;

	/* sections in the a.out data part: from the first one that can take
	   input from data location counters */
	{
		int data = 0;

		for (sect = sects; sect != NULL; sect = sect->next)
			sect->datapart = data = data || takesdata(sect->body);
	}

	/* the class of each symbol comes from its last assignment */
	for (s = top; s != NULL; s = s->next) {
		if (s->kind == S_SECTION) {
			struct stmt *t, *stack[16];
			int depth = 0;

			class = s->sect->datapart ? C_DATA : C_TEXT;
			t = s->sect->body;
			while (t != NULL || depth > 0) {
				if (t == NULL) {
					t = stack[--depth];
					continue;
				}
				if (t->kind == S_ASSIGN && t->sym != NULL)
					t->sym->class = class;
				if (t->kind == S_ONEPAGE && depth < 16) {
					stack[depth++] = t->next;
					t = t->body;
					continue;
				}
				t = t->next;
			}
		} else if (s->sym != NULL)
			s->sym->class = relative(s->e) ? class : C_ABS;
	}

	/* __name_start, __name_end and __name_load for each output section,
	   provided if a loaded module imports them */
	for (sect = sects; sect != NULL; sect = sect->next) {
		static const char *suffix[] = { "start", "end", "load" };
		char sname[80];
		int k;

		for (k = 0; k < 3; ++k) {
			struct expr *e;

			snprintf(sname, sizeof sname, "__%s_%s",
				 sect->name + (sect->name[0] == '.'),
				 suffix[k]);
			if (findssym(sname) != NULL || (symptr = findsym(sname))
			    == NULL || moddefined(symptr))
				continue;
			e = newexpr(X_FUNC);
			e->name = k == 2 ? "LOADADDR" : "ADDR";
			e->a = newexpr(X_SYM);
			e->a->name = sect->name;
			if (k == 1) {
				struct expr *size = newexpr(X_FUNC);

				size->name = "SIZEOF";
				size->a = e->a;
				e = newexpr(X_ADD);
				e->a = newexpr(X_FUNC);
				e->a->name = "ADDR";
				e->a->a = size->a;
				e->b = size;
			}
			s = newstmt(S_ASSIGN);
			s->akind = A_PROVIDE;
			s->sym = ssym(sname);
			s->sym->provide = 1;
			s->sym->class = sect->datapart ? C_DATA : C_TEXT;
			s->e = e;
			addtop(s);
		}
	}

	/* imports by loaded modules */
	for (modptr = modfirst; modptr != NUL_PTR; modptr = modptr->modnext)
		if (modptr->loadflag)
			for (symparray = modptr->symparray;
			     (symptr = *symparray) != NUL_PTR; ++symparray)
				if ((ss = findssym(symptr->name)) != NULL
				    && !moddefined(symptr))
					ss->referenced = 1;

	for (ss = ssyms; ss != NULL; ss = ss->next) {
		symptr = findsym(ss->name);
		ss->moddef = moddefined(symptr);
		if (ss->plain && ss->moddef)
			script_error(0, "%s is defined by both the script and %s",
				     ss->name, symptr->modptr->filename);
		ss->defined = ss->plain || (ss->provide && !ss->moddef
					    && ss->referenced);
		if (ss->defined && ss->referenced) {
			/* the import is satisfied by the script, from any
			   location counter; the value is set later */
			ss->symptr = symptr;
			symptr->flags = E_MASK | (ss->class == C_ABS ? A_MASK
						  : ss->class == C_DATA ? 3 : 0);
		}
	}
}

PRIVATE void inregion(struct osect *sect, long addr, struct region *r)
{
	if (addr < r->org || addr > r->org + r->len)
		script_error(sect->line, "section %s isn't in memory region %s",
			     sect->name, r->name);
	if (addr + sect->size > r->org + r->len)
		script_error(sect->line, "section %s doesn't fit in memory region "
			     "%s: it's %ld bytes too big", sect->name, r->name,
			     addr + sect->size - (r->org + r->len));
}

PRIVATE void checks(void)
{
	struct osect *a, *b;

	for (a = sects; a != NULL; a = a->next) {
		if (a->vma < 0 || a->vma + a->size > 0x10000L)
			script_error(a->line, "section %s goes past $FFFF",
				     a->name);
		if (a->lma < 0 || a->lma + a->size > 0x10000L)
			script_error(a->line, "section %s's load address goes "
				     "past $FFFF", a->name);
		if (a->region != NULL)
			inregion(a, a->vma, a->region);
		if (a->lmaregion != NULL)
			inregion(a, a->lma, a->lmaregion);
		if (a->lma != a->vma && !binary_output)
			script_error(a->line, "section %s has a separate load "
				     "address, which needs OUTPUT_FORMAT(binary)",
				     a->name);
		if (a->size == 0)
			continue;
		for (b = a->next; b != NULL; b = b->next)
			if (b->size != 0 && binary_output && !a->noload
			    && !b->noload && a->lma < b->lma + b->size
			    && b->lma < a->lma + a->size)
				script_error(b->line, "the load addresses of "
					     "sections %s and %s overlap",
					     a->name, b->name);
		for (b = a->next; b != NULL; b = b->next)
			if (b->size != 0
			    && (a->datapart == b->datapart || !sepid_option)
			    && a->vma < b->vma + b->size
			    && b->vma < a->vma + a->size)
				script_error(b->line,
					     "sections %s and %s overlap",
					     a->name, b->name);
	}
}

PUBLIC void script_layout(sepid, binary)
int sepid;
int binary;
{
	struct modstruct *modptr;
	struct osect *sect, *first[2], *prev[2];
	int i, seg;

	sepid_option = sepid;
	binary_output = binary;

	/* the input sections of the loaded modules */
	npieces = 0;
	for (modptr = modfirst; modptr != NUL_PTR; modptr = modptr->modnext)
		if (modptr->loadflag)
			npieces += 2 * NSEG;
	pieces = xalloc(npieces * sizeof *pieces);
	i = 0;
	for (modptr = modfirst; modptr != NUL_PTR; modptr = modptr->modnext)
		if (modptr->loadflag) {
			char *cptr = modptr->segsize;
			unsigned sizecount;

			for (seg = 0; seg < NSEG; ++seg) {
				pieces[i].mod = modptr;
				pieces[i].seg = seg;
				pieces[i].size = cntooffset(cptr,
					sizecount = segsizecount((unsigned)seg,
								 modptr));
				cptr += sizecount;
				++i;
				pieces[i].mod = modptr;
				pieces[i].seg = seg;
				pieces[i].common = 1;
				pieces[i].size = modptr->comsize[seg];
				++i;
			}
		}

	for (i = 0; ; ++i) {
		struct stmt *s;

		if (i == 0) {
			for (s = top; s != NULL; s = s->next)
				if (s->kind == S_SECTION)
					assign(s->sect->body, s->sect);
			orphans();
		}
		pass = i + 1;
		layout_pass();
		if (!remember() && i > 0)
			break;
		if (pass == MAXPASSES)
			script_error(0, "the layout doesn't settle: check for "
				     "symbols that depend on each other");
	}
	if (failed != NULL)
		script_error(failed->line, "%s", failed->message);

	/* the a.out text and data parts */
	first[0] = first[1] = prev[0] = prev[1] = NULL;
	for (sect = sects; sect != NULL; sect = sect->next) {
		int part = sect->datapart;

		/* (a binary image is laid out by load address instead) */
		if (!binary && prev[part] != NULL && sect->vma
		    < prev[part]->vma + prev[part]->size && sect->size != 0)
			script_error(sect->line, "section %s must not start "
				     "below the end of the section before it",
				     sect->name);
		if (first[part] == NULL)
			first[part] = sect;
		prev[part] = sect;
	}
	if (first[0] != NULL) {
		lay_text_start = first[0]->vma;
		lay_text_end = prev[0]->vma + prev[0]->size;
	} else
		lay_text_start = lay_text_end = first[1] ? first[1]->vma : 0;
	if (first[1] != NULL) {
		lay_data_start = first[1]->vma;
		lay_bss_end = prev[1]->vma + prev[1]->size;
	} else
		lay_data_start = lay_bss_end = lay_text_end;
	/* initialized data ends with the last data input in the data part */
	lay_data_end = lay_data_start;
	for (i = 0; i < nextras; ++i)
		if (extras[i].data && extras[i].sect->datapart
		    && !extras[i].sect->noload
		    && extras[i].vma + extras[i].len > lay_data_end)
			lay_data_end = extras[i].vma + extras[i].len;
	for (i = 0; i < npieces; ++i)
		if (!pieces[i].common && pieces[i].size != 0
		    && pieces[i].sect->datapart && !pieces[i].sect->noload) {
			long end = pieces[i].vma + pieces[i].size;
			long sub = pieces[i].sect->sub;

			end += (sub - pieces[i].size % sub) % sub;
			if (end > lay_data_end)
				lay_data_end = end;
		}
	lay_sepid = sepid || (first[1] != NULL && lay_data_start != lay_text_end);
	checks();

	/* where each module's part of each segment goes in the file: an
	   a.out has the text part followed by the data part, and a binary
	   image covers the load addresses from the lowest used */
	{
		long low = -1;

		for (sect = sects; sect != NULL; sect = sect->next)
			if (sect->size != 0 && !sect->noload
			    && (low < 0 || sect->lma < low))
				low = sect->lma;
		if (low < 0)
			low = lay_text_start;
		filelow = low;
		for (i = 0; i < npieces; ++i) {
			struct piece *pc = &pieces[i];

			if (pc->common || pc->sect->noload)
				continue;
			if (binary)
				pc->mod->segfadj[pc->seg] = pc->sect->lma
				    - pc->sect->vma - low;
			else if (pc->sect->datapart)
				pc->mod->segfadj[pc->seg] = lay_text_end
				    - lay_text_start - lay_data_start;
			else
				pc->mod->segfadj[pc->seg] = -lay_text_start;
		}
	}
}

/* output */

/* the file offset (after any header) of an address in a section */
PRIVATE long fileoff(struct osect *sect, long vma)
{
	if (binary_output)
		return sect->lma + (vma - sect->vma) - filelow;
	if (sect->datapart)
		return lay_text_end - lay_text_start + vma - lay_data_start;
	return vma - lay_text_start;
}

PRIVATE void outfill(void (*out) P((bin_off_t, char *, unsigned)),
		     long offset, long len, int byte)
{
	char buf[256];
	unsigned n;

	memset(buf, byte, sizeof buf);
	while (len > 0) {
		n = len > (long)sizeof buf ? sizeof buf : (unsigned)len;
		(*out) ((bin_off_t) offset, buf, n);
		offset += n;
		len -= n;
	}
}

/* fill bytes for gaps, written before the modules */
PUBLIC void script_write_fill(out)
void (*out) P((bin_off_t offset, char *buf, unsigned len));
{
	struct osect *sect, *a, *b;
	long fill, end;
	int i;

	/* gaps inside sections */
	for (i = 0; i < nextras; ++i)
		if (!extras[i].data && !extras[i].sect->noload)
			outfill(out, fileoff(extras[i].sect, extras[i].vma),
				extras[i].len, (int)extras[i].value);

	/* gaps between sections, in file order */
	if (globalfill == NULL || (fill = fillbyte(globalfill)) == 0)
		return;
	for (a = sects; a != NULL; a = a->next) {
		if (a->size == 0 || a->noload)
			continue;
		end = fileoff(a, a->vma) + a->size;
		b = NULL;
		for (sect = sects; sect != NULL; sect = sect->next)
			if (sect->size != 0 && !sect->noload
			    && fileoff(sect, sect->vma) >= end
			    && (b == NULL || fileoff(sect, sect->vma)
				< fileoff(b, b->vma)))
				b = sect;
		if (b != NULL && fileoff(b, b->vma) > end)
			outfill(out, end, fileoff(b, b->vma) - end, (int)fill);
	}
}

/* BYTE and WORD data, written after the modules */
PUBLIC void script_write_data(out)
void (*out) P((bin_off_t offset, char *buf, unsigned len));
{
	char buf[2];
	int i;

	for (i = 0; i < nextras; ++i)
		if (extras[i].data && !extras[i].sect->noload) {
			if (extras[i].len == 1)
				buf[0] = (char)extras[i].value;
			else {
				buf[0] = (char)(extras[i].value >> 8);
				buf[1] = (char)extras[i].value;
			}
			(*out) ((bin_off_t) fileoff(extras[i].sect,
						    extras[i].vma),
				buf, (unsigned)extras[i].len);
		}
}

/* module selection: modules that KEEP patterns match, and those that
   define symbols an ASSERT uses, are linked like the entry point's */
PRIVATE void exprroots(struct expr *e, void (*link) P((struct modstruct *)))
{
	struct symstruct *symptr;

	if (e == NULL)
		return;
	if (e->op == X_SYM && (symptr = findsym(e->name)) != NULL
	    && !(symptr->flags & I_MASK) && symptr->modptr != NULL
	    && !symptr->modptr->loadflag)
		(*link) (symptr->modptr);
	exprroots(e->a, link);
	exprroots(e->b, link);
	exprroots(e->c, link);
}

PRIVATE void stmtroots(struct stmt *s, void (*link) P((struct modstruct *)))
{
	struct modstruct *modptr;
	int i, seg;

	for (; s != NULL; s = s->next) {
		if (s->kind == S_SECTION)
			stmtroots(s->sect->body, link);
		else if (s->kind == S_ONEPAGE)
			stmtroots(s->body, link);
		else if (s->kind == S_ASSERT)
			exprroots(s->e, link);
		else if (s->kind == S_INPUT && s->keep)
			for (modptr = modfirst; modptr != NUL_PTR;
			     modptr = modptr->modnext) {
				char *cptr = modptr->segsize;
				unsigned count;
				int match = 0;

				if (modptr->loadflag
				    || !filematch(s->filepat, modptr))
					continue;
				for (seg = 0; seg < NSEG; ++seg) {
					long size = cntooffset(cptr,
						count = segsizecount(
						    (unsigned)seg, modptr));

					cptr += count;
					for (i = 0; i < s->nsecpats; ++i)
						if ((size != 0
						     && secmatch(s->secpats[i],
								 seg, 0))
						    || secmatch(s->secpats[i],
								seg, 1))
							match = 1;
				}
				if (match)
					(*link) (modptr);
			}
	}
}

PUBLIC void script_roots(link)
void (*link) P((struct modstruct * modptr));
{
	stmtroots(top, link);
}

/* the value of a symbol the script assigns, even if it isn't defined
   for the modules (a PROVIDE that nothing uses) */
PUBLIC int script_value(name, value)
char *name;
bin_off_t *value;
{
	struct ssym *ss = findssym(name);

	if (ss == NULL || !ss->assigned || ss->hidden)
		return 0;
	*value = ss->value;
	return 1;
}

/* set the values of the imports the script satisfies */
PUBLIC void script_define_symbols()
{
	struct ssym *ss;

	for (ss = ssyms; ss != NULL; ss = ss->next)
		if (ss->symptr != NULL)
			ss->symptr->value = ss->value;
}

/* write the script's symbols that no loaded module imports */
PUBLIC void script_write_symbols(writesym)
void (*writesym) P((char *name, bin_off_t value, int sclass));
{
	struct ssym *ss;
	int order;

	for (order = 0; order < nssyms; ++order)
		for (ss = ssyms; ss != NULL; ss = ss->next)
			if (ss->order == order && ss->defined && !ss->referenced)
				(*writesym) (ss->name, ss->value, ss->class);
}

/* -M: the layout */

PUBLIC void script_dump_layout()
{
	struct osect *sect;
	struct stmt *s, *stack[16];
	int depth, i;

	struct region *r;

	if (regions != NULL) {
		putstr("Memory region   Origin   Length   Used\n");
		for (r = regions; r != NULL; r = r->next) {
			putstr(r->name);
			for (i = strlen(r->name); i < 16; ++i)
				putstr(" ");
			put08x((bin_off_t) r->org);
			putstr(" ");
			put08x((bin_off_t) r->len);
			putstr(" ");
			put08x((bin_off_t) (r->cursor - r->org));
			putstr("\n");
		}
		putstr("\n");
	}
	putstr("Section         Address  Size     Load\n");
	for (sect = sects; sect != NULL; sect = sect->next) {
		putstr(sect->name);
		for (i = strlen(sect->name); i < 16; ++i)
			putstr(" ");
		put08x((bin_off_t) sect->vma);
		putstr(" ");
		put08x((bin_off_t) sect->size);
		if (sect->lma != sect->vma) {
			putstr(" ");
			put08x((bin_off_t) sect->lma);
		}
		if (sect->noload)
			putstr(" (NOLOAD)");
		putstr("\n");
		depth = 0;
		s = sect->body;
		while (s != NULL || depth > 0) {
			if (s == NULL) {
				s = stack[--depth];
				continue;
			}
			if (s->kind == S_ONEPAGE && depth < 16) {
				stack[depth++] = s->next;
				s = s->body;
				continue;
			}
			if (s->kind == S_INPUT)
				for (i = 0; i < s->npieces; ++i)
					if (s->pieces[i]->size != 0) {
						putstr("    ");
						put08x((bin_off_t) s->pieces[i]->vma);
						putstr(" ");
						put08x((bin_off_t) s->pieces[i]->size);
						putstr(" ");
						putstr(piecename(s->pieces[i]));
						putstr("\n");
					}
			s = s->next;
		}
	}
	putstr("\n");
}
