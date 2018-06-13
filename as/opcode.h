/* opcode.h - routine numbers and special opcodes for assembler */

enum
{
/* Pseudo-op routine numbers.
 * Conditionals are first - this is used to test if op is a conditional.
 */
    ELSEOP,
    ELSEIFOP,
    ELSEIFCOP,
    ENDIFOP,
    IFOP,
    IFCOP,

#define MIN_NONCOND	ALIGNOP
    ALIGNOP,
    ASCIZOP,
    BLKWOP,
    BLOCKOP,
    BSSOP,
    COMMOP,
    COMMOP1,
    DATAOP,
    ENDBOP,
    ENTEROP,
    ENTRYOP,
    EQUOP,
    EVENOP,
    EXPORTOP,
    FAILOP,
    FCBOP,
    FCCOP,
    FDBOP,
#if SIZEOF_OFFSET_T > 2
    FQBOP,
#endif
    GETOP,
    GLOBLOP,
    IDENTOP,
    IMPORTOP,
    LCOMMOP,
    LCOMMOP1,
    LISTOP,
    LOCOP,
    MACLISTOP,
    MACROOP,
    MAPOP,
    ORGOP,
    PROCEOFOP,
    RMBOP,
    SECTOP,
    SETOP,
    SETDPOP,
    TEXTOP,
    WARNOP,

/* Machine-op routine numbers. */

    ALL,			/* all address modes allowed, like LDA */
    ALTER,			/* all but immediate, like STA */
    IMMED,			/* immediate only (ANDCC, ORCC) */
    INDEXD,			/* indexed (LEA's) */
    INHER,			/* inherent, like CLC or CLRA */
    LONG,			/* long branches */
    SHORT,			/* short branches */
    SSTAK,			/* S-stack (PSHS, PULS) */
    SWAP,			/* TFR, EXG */
    USTAK			/* U-stack (PSHU,PULU) */
};

/* Special opcodes. */

# define JMP_OPCODE		0x7E
# define JSR_OPCODE		0xBD
# define PAGE1_OPCODE		0x10
# define PAGE2_OPCODE		0x11
