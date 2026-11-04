/* srec.h - Motorola S-record and Intel hex output, shared by ld01 and
   objcopy01 */

#include <stdio.h>

void srec_header(FILE *f, const char *name);
unsigned srec_data(FILE *f, const unsigned char *buf, unsigned len,
		   unsigned address);
void srec_end(FILE *f, unsigned records, unsigned entry);

void ihex_data(FILE *f, const unsigned char *buf, unsigned len,
	       unsigned address);
void ihex_end(FILE *f, unsigned entry);
