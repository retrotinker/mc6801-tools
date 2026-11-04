/* script.h - linker scripts */

/* set up the script: the default one, or one read from a file */
void script_use_default P((int sepid, int binary));
void script_read P((char *filename));
void script_print_default P((int sepid, int binary));
void script_define P((char *assignment));	/* -A sym=value */

/* steps of a link, in order */
void script_set_entry P((void));	/* before modules are selected */
void script_mark_imports P((void));	/* before symbols are relocated */
void script_layout P((int sepid, int binary));	/* place everything */
void script_define_symbols P((void));	/* after symbols are relocated */
void script_write_symbols P((void (*writesym) (char *name, bin_off_t value,
					      int sclass)));
void script_dump_layout P((void));	/* -M */
void script_write_fill P((void (*out) (bin_off_t offset, char *buf,
				     unsigned len)));
void script_write_data P((void (*out) (bin_off_t offset, char *buf,
				     unsigned len)));
#ifdef OBJ_H
void script_roots P((void (*link) (struct modstruct *modptr)));
#endif
int script_value P((char *name, bin_off_t *value));	/* if assigned */
void script_convert P((char *outfilename, bin_off_t entry));

extern int script_binary;		/* no header: binary, srec or ihex */
extern int script_records;		/* OUTPUT_FORMAT(srec) or (ihex) */
#define RECORDS_SREC	1
#define RECORDS_IHEX	2
extern int script_orphan_error;		/* -Z */
extern int text_base_given;		/* -T was given */

/* the layout, for the output writer */
extern bin_off_t lay_text_start;	/* a.out text part */
extern bin_off_t lay_text_end;
extern bin_off_t lay_data_start;	/* a.out data part */
extern bin_off_t lay_data_end;		/* end of initialized data */
extern bin_off_t lay_bss_end;		/* end of the data part */
extern int lay_sepid;			/* separate instruction and data */

/* symbol classes passed to script_write_symbols' function */
#define SCRIPT_TEXT	0
#define SCRIPT_DATA	1
#define SCRIPT_ABS	2
