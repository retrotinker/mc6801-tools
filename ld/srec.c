/*
 * srec.c - Motorola S-record and Intel hex output, shared by ld01 and
 * objcopy01.  Data records hold up to 16 bytes.
 *
 * Copyright 2014 John W. Linville <linville@tuxdriver.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <string.h>
#include "srec.h"

#define LINESIZE	16

/* S0: a header record holding a name */
void srec_header(FILE *f, const char *name)
{
	unsigned len = strlen(name);
	unsigned char csum = len + 3;
	unsigned i;

	fprintf(f, "S0%02X0000", len + 3);
	for (i = 0; i < len; i++) {
		fprintf(f, "%02X", (unsigned char)name[i]);
		csum += (unsigned char)name[i];
	}
	fprintf(f, "%02X\n", csum ^ 0xff);
}

/* S1: data records; returns how many were written */
unsigned srec_data(FILE *f, const unsigned char *buf, unsigned len,
		   unsigned address)
{
	unsigned i, n, records = 0;
	unsigned char csum;

	while (len > 0) {
		n = len > LINESIZE ? LINESIZE : len;
		fprintf(f, "S1%02X%04X", n + 3, address & 0xffff);
		csum = n + 3 + ((address >> 8) & 0xff) + (address & 0xff);
		for (i = 0; i < n; i++) {
			csum += buf[i];
			fprintf(f, "%02X", buf[i]);
		}
		fprintf(f, "%02X\n", csum ^ 0xff);
		buf += n;
		len -= n;
		address += n;
		records++;
	}
	return records;
}

/* S5: the number of data records; S9: the entry point */
void srec_end(FILE *f, unsigned records, unsigned entry)
{
	unsigned char csum;

	fprintf(f, "S503%04X", records & 0xffff);
	csum = 3 + ((records >> 8) & 0xff) + (records & 0xff);
	fprintf(f, "%02X\n", csum ^ 0xff);

	fprintf(f, "S903%04X", entry & 0xffff);
	csum = 3 + ((entry >> 8) & 0xff) + (entry & 0xff);
	fprintf(f, "%02X\n", csum ^ 0xff);
}

/* Intel hex data records */
void ihex_data(FILE *f, const unsigned char *buf, unsigned len,
	       unsigned address)
{
	unsigned i, n;
	unsigned char csum;

	while (len > 0) {
		n = len > LINESIZE ? LINESIZE : len;
		fprintf(f, ":%02X%04X00", n, address & 0xffff);
		csum = n + ((address >> 8) & 0xff) + (address & 0xff);
		for (i = 0; i < n; i++) {
			csum += buf[i];
			fprintf(f, "%02X", buf[i]);
		}
		fprintf(f, "%02X\r\n", (unsigned char)((csum ^ 0xff) + 1));
		buf += n;
		len -= n;
		address += n;
	}
}

/* Intel hex end record, holding the entry point */
void ihex_end(FILE *f, unsigned entry)
{
	unsigned char csum = 1 + ((entry >> 8) & 0xff) + (entry & 0xff);

	fprintf(f, ":00%04X01%02X\r\n", entry & 0xffff,
		(unsigned char)((csum ^ 0xff) + 1));
}
