#define align(x)		/* ((x) = ((int) (x) + (4-1)) & ~(4-1)) */
#define LOW_BYTE 0		/* must be changed for big-endian */

/* const.h - constants for assembler */

/* major switches */

#undef  I80386			/* generate 80386 code */
#define MC6809			/* generate 6809 code */
#define MNSIZE			/* allow byte size in mnemonic, e.g. "movb" */
#undef  SOS_EDOS		/* source OS is EDOS */

/* defaults */

#define DIRCHAR '/'		/* character separating filename from dir */
#define INBUFSIZE 8192
#define SOS_EOLSTR "\012"

/* defaults modified by switches */


/* booleans */

#define FALSE 0
#define TRUE 1

/* ASCII constants */

#define ETB  23

/* C tricks */

#define EXTERN extern
#define FORWARD static
#define PRIVATE static
#define PUBLIC
#define NULL 0

/* O/S constants */

#define CREAT_PERMS 0666
#define EOF (-1)
#define STDIN 0
#define STDOUT 1

/* register codes (internal to assembler) */



/* index regs must be first, then PC, then other regs */

#define AREG  5
#define BREG  6
#define CCREG 7
#define DPREG 8
#define DREG  9
#define MAXINDREG 3
#define NOREG 10
#define PCREG 4
#define SREG  0
#define UREG  1
#define XREG  2
#define YREG  3



/* special chars */

#define EOL		0
#define MACROCHAR	'?'

/* symbol codes */

/* the first 2 must be from chars in identifiers */
#define IDENT		0
#define INTCONST	1

/* the next few are best for other possibly-multi-char tokens */
#define ADDOP		2	/* also ++ */
#define BINCONST	3
#define CHARCONST	4
#define GREATERTHAN	5	/* also >> and context-sensitive */
#define HEXCONST	6
#define LESSTHAN	7	/* also << and context-sensitive */
#define SUBOP		8	/* also -- */
#define WHITESPACE	9

#define ANDOP		10
#define COMMA		11
#define EOLSYM		12
#define EQOP		13
#define IMMEDIATE	14
#define INDIRECT	15
#define LBRACKET	16
#define LPAREN		17
#define MACROARG	18
#define NOTOP		19
#define OROP		20
#define OTHERSYM	21
#define POSTINCOP	22
#define PREDECOP	23
#define RBRACKET	24
#define RPAREN		25
#define SLASH		26	/* context-sensitive */
#define SLOP		27
#define SROP		28
#define STAR		29	/* context-sensitive */
#define STRINGCONST	30
#define COLON		31

/* these are from assembler errors module */

/* syntax errors */

#define COMEXP 0
#define DELEXP 1
#define FACEXP 2
#define IREGEXP 3
#define LABEXP 4
#define LPEXP 5
#define OPEXP 6
#define RBEXP 7
#define REGEXP 8
#define RPEXP 9
#define SPEXP 10

/* expression errors */

#define ABSREQ 11
#define NONIMPREQ 12
#define RELBAD 13

/* label errors */

#define ILLAB 14
#define MACUID 15
#define MISLAB 16
#define MNUID 17
#define REGUID 18
#define RELAB 19
#define UNBLAB 20
#define UNLAB 21
#define VARLAB 22

/* addressing errors */

#define ABOUNDS 23
#define DBOUNDS 24
#define ILLMOD 25
#define ILLREG 26

/* control structure errors */

#define ELSEBAD 27
#define ELSEIFBAD 27
#define ENDBBAD 28
#define ENDIFBAD 27
#define EOFBLOCK 29
#define EOFIF 30

#define EOFLC 31
#define EOFMAC 32
#define FAILERR 33

/* overflow errors */

#define BLOCKOV 34
#define BWRAP 35
#define COUNTOV 36
#define COUNTUN 37
#define GETOV 38
#define IFOV 39

#define LINLONG 40
#define MACOV 41
#define OBJSYMOV 42
#define OWRITE 43
#define PAROV 44
#define SYMOV 45
#define SYMOUTOV 46

/* i/o errors */

#define OBJOUT 47

/* miscellaneous errors */

#define CTLINS 48
#define FURTHER 49
#define NOIMPORT 50
#define NOTIMPLEMENTED 51
#define REENTER 52
#define SEGREL 53

/* warnings */

#define MINWARN 54
#define ALREADY 54
#define SHORTB 55

/* symbol table entry */

				/* type entry contains following flags */
#define ENTBIT   (1<<0)		/* entry point (=OBJ_N_MASK) */
#define COMMBIT  (1<<1)		/* common */
#define LABIT    (1<<2)		/* label (a PC location or defined by EQU) */
#define MNREGBIT (1<<3)		/* mnemonic for op or pseudo-op, or register */
#define MACBIT   (1<<4)		/* macro */
#define REDBIT   (1<<5)		/* redefined */
#define VARBIT   (1<<6)		/* variable (i.e. something defined by SET) */
#define EXPBIT   (1<<7)		/* exported (= OBJ_E_MASK) */

				/* data entry contains following flags, valid */
				/* for expressions as well as syms */
#define PAGE1    (1<<0)		/* page 1 machine op = MNREGBIT \ PAGE1 */
#define PAGE2    (1<<1)		/* page 2 machine op = MNREGBIT \ PAGE2 */
#define REGBIT   (1<<2)		/* register = MNREGBIT \ REGBIT */
#define SIZEBIT  (1<<3)		/* sizing mnemonic = MNREGBIT \ SIZEBIT */
#define SEGM     15		/* 1st 4 bits reused for segment if !MNREGBIT */
#define RELBIT   (1<<4)		/* relative (= OBJ_A_MASK) */
#define FORBIT   (1<<5)		/* forward referenced */
#define IMPBIT   (1<<6)		/* imported (= OBJ_I_MASK) */
#define UNDBIT   (1<<7)		/* undefined */

/* pseudo-op routine numbers */
/* conditionals are first, this is used to test if op is a conditional */

#define ELSEOP      0
#define ELSEIFOP    1
#define ELSEIFCOP   2
#define ENDIFOP     3
#define IFOP        4
#define IFCOP       5
#define MAXCOND     6		/* limit of conditionals */

#define BLOCKOP     6
#define COMMOP      7
#define ENDOP       8
#define ENDBOP      9
#define ENTEROP    10
#define ENTRYOP    11
#define EQUOP      12
#define EXPORTOP   13
#define FAILOP     14
#define FCBOP      15
#define FCCOP      16
#define FDBOP      17
#define GETOP      18
#define IDENTOP    19
#define IMPORTOP   20
#define _LISTOP    21
#define LOCOP      22
#define _MACLISTOP 23
#define MACROOP    24
#define _MAPOP     25
#define ORGOP      26
#define RMBOP      27
#define SETOP      28
#define SETDPOP    29
#define _WARNOP    30



/* machine-op routine numbers */

#define ALL        31		/* all address modes allowed, like LDA */
#define ALTER      32		/* all but immediate, like STA */
#define IMMED      33		/* immediate only (ANDCC, ORCC) */
#define INDEXD     34		/* indexed (LEA's) */
#define INHER      35		/* inherent, like CLC or CLRA */
#define LONG       36		/* long branches */
#define SHORT      37		/* short branches */
#define SSTAK      38		/* S-stack (PSHS, PULS) */
#define SWAP       39		/* TFR, EXG */
#define USTAK      40		/* U-stack (PSHU,PULU) */

/* yet further pseudo-ops */

#define LCOMMOP    41


/* object code format (Introl) */

#define OBJ_SEGSZ_TWO  0x02	/* size 2 code for segment size descriptor */

#define OBJ_MAX_ABS_LEN  64	/* max length of chunk of absolute code */

#define OBJ_ABS        0x40	/* absolute code command */
#define OBJ_OFFSET_REL 0x80	/* offset relocation command */
#define OBJ_SET_SEG    0x20	/* set segment command */
#define OBJ_SKIP_1     0x11	/* skip with 1 byte count */
#define OBJ_SKIP_2     0x12	/* skip with 2 byte count */
#define OBJ_SKIP_4     0x13	/* skip with 4 byte count */
#define OBJ_SYMBOL_REL 0xC0	/* symbol relocation command */

#define OBJ_A_MASK     0x10	/* absolute bit(symbols) */
#if OBJ_A_MASK - RELBIT		/* must match internal format (~byte 1 -> 0) */
oops - RELBIT misplaced
#endif
#define OBJ_E_MASK     0x80	/* exported bit (symbols) */
#if OBJ_E_MASK - EXPBIT		/* must match internal format (byte 0 -> 0) */
oops - EXPBIT misplaced
#endif
#define OBJ_I_MASK     0x40	/* imported bit (symbols) */
#if OBJ_I_MASK - IMPBIT		/* must match internal format (byte 1 -> 0) */
oops - IMPBIT misplaced
#endif
#define OBJ_N_MASK     0x01	/* entry bit (symbols) */
#if OBJ_N_MASK - ENTBIT		/* must match internal format (byte 0 -> 1) */
oops - ENTBIT misplaced
#endif
#define OBJ_SA_MASK    0x20	/* size allocation bit (symbols) */
#define OBJ_SZ_ONE     0x40	/* size one code for symbol value */
#define OBJ_SZ_TWO     0x80	/* size two code for symbol value */
#define OBJ_SZ_FOUR    0xC0	/* size four code for symbol value */

#define OBJ_R_MASK     0x20	/* PC-rel bit (off & sym reloc commands) */
#define OBJ_SEGM_MASK  0x0F	/* segment mask (symbols, off reloc command) */

#define OBJ_OF_MASK    0x03	/* offset size code for symbol reloc */
#define OBJ_S_MASK     0x04	/* symbol number size code for symbol reloc */

#define SYMLIS_NAMELEN 26
#define SYMLIS_LEN (sizeof (struct sym_listing_s))

#define FILNAMLEN 64		/* max length of a file name */
#define LINLEN 256		/* max length of input line */
#define LINUM_LEN 5		/* length of formatted line number */

#define SPTSIZ 1024		/* number of symbol ptrs */
				/* pseudo-op flags */
#define POPHI 1			/* set to print hi byte of adr */
#define POPLO 2			/* to print lo byte of ADR */
#define POPLC 4			/* to print LC */
#define POPLONG 8		/* to print high word of ADR */
#define MAXBLOCK 8		/* max nesting level of BLOCK stack */
#define MAXGET 8		/* max nesting level of GET stack */
#define MAXIF 8			/* max nesting level of IF stack */
#define MACPSIZ (128/sizeof (struct schain_s))
				/* size of macro param buffer */
#define MAXMAC 8		/* max nesting level of macro stack */
#define NLOC 16			/* number of location counters */

/* special segments */

#define BSSLOC 3
#define DATALOC 3
#define DPLOC 2
#define STRLOC 1
#define TEXTLOC 0
