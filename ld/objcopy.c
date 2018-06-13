/*
 * Copyright 2014 John W. Linville <linville@tuxdriver.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

#include "x86_aout.h"

#define SREC_LINESIZE	16
#define IHEX_LINESIZE	16
#define CAS_BLOCKSIZE	255
#define CAS_LEADERSIZE	128
#define WAV_HZ		43200

enum output_formats {
	BINARY,		/* raw binary (no header) */
	DECB_BIN,	/* Disk Extend Color BASIC BIN file */
	DDOS_BIN,	/* Dragon DOS BIN file */
	OS9_MODULE,	/* OS-9/6809 program module file */
	SREC,		/* Motorola S-record file */
	IHEX,		/* Intel Hex file */
	CAS,		/* Color BASIC CAS file (for both CoCo and Dragon) */
	WAV,		/* Color BASIC cassete WAV file */
};

unsigned int textaddr;
unsigned int dataaddr;
unsigned int datasize;
unsigned int objversion;
unsigned int reentrant;
unsigned int padoutput;
char *objname;

uint8_t os9crc[3] = { 0xff, 0xff, 0xff };

uint32_t textstart;
uint32_t textend;
uint32_t datastart;

struct {
	char *n_name;
	uint32_t *val;
} searchsyms[] = {
	{
		.n_name = "__btext",
		.val = &textstart,
	},
	{
		.n_name = "__etext",
		.val = &textend,
	},
	{
		.n_name = "__bdata",
		.val = &datastart,
	},
};
#define NUM_SEARCH_SYMS	(sizeof(searchsyms) / sizeof(searchsyms[0]))

static char *progname;

void usage(void)
{
	fprintf(stderr,
		"Usage: %s [-O <output type>] [-n <object name>]\n"
		"\t\t[-T <text address>] [-D <data address>]\n"
		"\t\t[-v <object version>] [-d <data size>] [-rp]\n"
		"\t\tinfile outfile\n", progname);
	exit(EXIT_FAILURE);
}

void fatal(char *str)
{
	fprintf(stderr, "%s: %s\n", progname, str);
	exit(EXIT_FAILURE);
}

void warning(char *str)
{
	fprintf(stderr, "%s: %s\n", progname, str);
}

void match_syms(FILE *ifd, struct exec header)
{
	struct nlist symbol;
	int i;
	int numsyms = header.a_syms / sizeof(struct nlist);
	int numsearchsyms = NUM_SEARCH_SYMS;

	if (fseek(ifd, A_SYMPOS(header), 0) < 0)
		fatal("Cannot seek to start of symbols");

	while (numsearchsyms && numsyms--) {
		if (fread(&symbol, sizeof(symbol), 1, ifd) != 1)
			fatal("Cannot read symbol information!");

		for (i = 0; i < NUM_SEARCH_SYMS; i++) {
			if (!strncmp(symbol.n_name, searchsyms[i].n_name,
				     sizeof(symbol.n_name))) {
				*searchsyms[i].val = be32toh(symbol.n_value);
				numsearchsyms--;
			}
		}
	}
}

void os9_crc_adjust(uint8_t *buffer, int ssize)
{
	int i;
	uint8_t datum;

	for (i = 0; i < ssize; i++) {
		datum = *buffer++;

		/*
		 * Not sure I grok the original
		 * CRC algorithm...  The algorithm
		 * below is based upon assembly code
		 * in the NitrOS-9 repository...
		 */
		datum ^= os9crc[0];
		os9crc[0]  = os9crc[1];
		os9crc[1]  = os9crc[2];
		os9crc[1] ^= (datum >> 7);
		os9crc[2]  = (datum << 1);
		os9crc[1] ^= (datum >> 2);
		os9crc[2] ^= (datum << 6);
		datum ^= (datum << 1);
		datum ^= (datum << 2);
		datum ^= (datum << 4);

		if (datum & 0x80) {
			os9crc[0] ^= 0x80;
			os9crc[2] ^= 0x21;
		}
	}
}

void write_file(FILE *ifd, FILE *ofd, unsigned int bsize)
{
	unsigned char buffer[1024];
	unsigned int ssize;

	while (bsize > 0) {
		if (bsize > sizeof(buffer))
			ssize = sizeof(buffer);
		else
			ssize = bsize;

		if ((ssize = fread(buffer, 1, ssize, ifd)) <= 0)
			fatal("Error reading segment from executable");

		os9_crc_adjust(buffer, ssize);
		if (fwrite(buffer, 1, ssize, ofd) != ssize)
			fatal("Error writing output file");
		bsize -= ssize;
	}
}

void write_zeroes(FILE *ofd, unsigned int bsize)
{
	unsigned char buffer[1024];
	unsigned int ssize;

	memset(buffer, 0, sizeof(buffer));
	while (bsize > 0) {
		if (bsize > sizeof(buffer))
			ssize = sizeof(buffer);
		else
			ssize = bsize;

		os9_crc_adjust(buffer, ssize);
		if (fwrite(buffer, 1, ssize, ofd) != ssize)
			fatal("Error writing zeroes to output file");
		bsize -= ssize;
	}
}

void raw_output(FILE *ifd, FILE *ofd, struct exec header)
{
	if (!textaddr)
		textaddr = textstart;
	if (!textend)
		textend = textstart + header.a_text;
	if (!dataaddr)
		if (header.a_flags & A_SEP)
			dataaddr = datastart;
		else
			dataaddr = textend;

	if (fseek(ifd, A_TEXTPOS(header), 0) < 0)
		fatal("Cannot seek to start of text");

	write_file(ifd, ofd, header.a_text);

	if (fseek(ifd, A_DATAPOS(header), 0) < 0)
		fatal("Cannot seek to start of data");

	if (padoutput && fseek(ofd, dataaddr - textaddr, 0) < 0)
		fatal("Cannot seek to start of data in outfile");

	write_file(ifd, ofd, header.a_data);
}

void decb_output(FILE *ifd, FILE *ofd, struct exec header)
{
	uint8_t binhead[5], binfoot[5];
	int binsize;

	if (!textaddr)
		textaddr = textstart;
	if (!textaddr)
		warning("BIN load address is zero");

	binsize = header.a_text;
	if (!(header.a_flags & A_SEP) ||
	    (!dataaddr && datastart == textend))
		binsize += header.a_data;

	binhead[0] = 0;
	binhead[1] = (binsize & 0xff00) >> 8;
	binhead[2] =  binsize & 0x00ff;
	binhead[3] = (textaddr & 0xff00) >> 8;
	binhead[4] =  textaddr & 0x00ff;

	if (fwrite(binhead, 1, sizeof(binhead), ofd) != sizeof(binhead))
		fatal("Error writing DECB BIN header to outfile");

	if (fseek(ifd, A_TEXTPOS(header), 0) < 0)
		fatal("Cannot seek to start of text");

	write_file(ifd, ofd, header.a_text);

	if ((header.a_flags & A_SEP) &&
	    (dataaddr || datastart != textend)) {
		if (!dataaddr)
			dataaddr = datastart;
		if (!dataaddr)
			warning("BIN data load address is zero");

		binsize = header.a_data;

		binhead[0] = 0;
		binhead[1] = (binsize & 0xff00) >> 8;
		binhead[2] =  binsize & 0x00ff;
		binhead[3] = (dataaddr & 0xff00) >> 8;
		binhead[4] =  dataaddr & 0x00ff;

		if (fwrite(binhead, 1, sizeof(binhead), ofd) != sizeof(binhead))
			fatal("Error writing DECB BIN data header to outfile");
	}

	if (fseek(ifd, A_DATAPOS(header), 0) < 0)
		fatal("Cannot seek to start of data");

	write_file(ifd, ofd, header.a_data);

	binfoot[0] = 0xff;
	binfoot[1] = binfoot[2] = 0;
	binfoot[3] = (header.a_entry & 0xff00) >> 8;
	binfoot[4] =  header.a_entry & 0x00ff;

	if (fwrite(binfoot, 1, sizeof(binfoot), ofd) != sizeof(binfoot))
		fatal("Error writing DECB BIN footer to outfile");
}

void ddos_output(FILE *ifd, FILE *ofd, struct exec header)
{
	uint8_t binhead[9];
	int binsize;

	if (!textaddr)
		textaddr = textstart;
	if (!textaddr)
		warning("BIN load address is zero");

	if ((header.a_flags & A_SEP) &&
	    (dataaddr || datastart != textend))
		fatal("Dragon DOS does not support split text/data");

	binsize = header.a_text + header.a_data;

	binhead[0] = 0x55;
	binhead[1] = 0x02;
	binhead[2] = (textaddr & 0xff00) >> 8;
	binhead[3] =  textaddr & 0x00ff;
	binhead[4] = (binsize & 0xff00) >> 8;
	binhead[5] =  binsize & 0x00ff;
	binhead[6] = (header.a_entry & 0xff00) >> 8;
	binhead[7] =  header.a_entry & 0x00ff;
	binhead[8] = 0xAA;

	if (fwrite(binhead, 1, sizeof(binhead), ofd) != sizeof(binhead))
		fatal("Error writing Dragon DOS BIN header to outfile");

	if (fseek(ifd, A_TEXTPOS(header), 0) < 0)
		fatal("Cannot seek to start of text");

	write_file(ifd, ofd, header.a_text);

	if (fseek(ifd, A_DATAPOS(header), 0) < 0)
		fatal("Cannot seek to start of data");

	write_file(ifd, ofd, header.a_data);
}

void os9_output(FILE *ifd, FILE *ofd, struct exec header)
{
	uint8_t os9hdr[13] = {
		0x87, 0xcd,
	};
	unsigned short modsize, heapsize;
	unsigned int i, namelen, nameoffset;
	unsigned char hdrchk = 0;

	namelen = strlen(objname);

	heapsize = header.a_total - header.a_text -
			header.a_data - header.a_bss;
	if (datasize < heapsize)
		datasize = heapsize;
	if (!datasize)
		fatal("OS-9 module data size is zero");

	/* can claim reentrant if no r/w static data allocations */
	if (!reentrant)
		reentrant = !(header.a_data + header.a_bss);

	nameoffset = sizeof(os9hdr) + header.a_text +
			header.a_data + header.a_bss;
	modsize = nameoffset + namelen + sizeof(os9crc);

	*((unsigned short *)&os9hdr[2]) = htobe16(modsize);
	*((unsigned short *)&os9hdr[4]) = htobe16(nameoffset);

	os9hdr[6] = 0x11; /* 6809 executable */

	if (objversion > 0x7f)
		fatal("OS-9 module version is too large");
	os9hdr[7] = objversion;
	if (reentrant)
		os9hdr[7] |= 0x80;

	for (i = 0; i < 8; i++)
		hdrchk ^= os9hdr[i];
	os9hdr[8] = hdrchk ^ 0xff;

	*((unsigned short *)&os9hdr[9]) =
		htobe16(sizeof(os9hdr) + header.a_entry);

	/* Request stack + parameter storage */
	*((unsigned short *)&os9hdr[11]) = htobe16(datasize);

	os9_crc_adjust(os9hdr, sizeof(os9hdr));
	if (fwrite(os9hdr, 1, sizeof(os9hdr), ofd) != sizeof(os9hdr))
		fatal("Error writing OS-9 module header to outfile");

	if (fseek(ifd, A_TEXTPOS(header), 0) < 0)
		fatal("Cannot seek to start of text");

	write_file(ifd, ofd, header.a_text);

	if (fseek(ifd, A_DATAPOS(header), 0) < 0)
		fatal("Cannot seek to start of data");

	write_file(ifd, ofd, header.a_data);

	write_zeroes(ofd, header.a_bss);

	/* End of string needs MSB set... */
	objname[namelen - 1] |= 0x80;

	os9_crc_adjust((uint8_t *)objname, namelen);
	if (fwrite(objname, 1, namelen, ofd) != namelen)
		fatal("Error writing OS-9 module name to outfile");

	os9crc[0] ^= 0xff;
	os9crc[1] ^= 0xff;
	os9crc[2] ^= 0xff;

	if (fwrite(os9crc, 1, sizeof(os9crc), ofd) != sizeof(os9crc))
		fatal("Error writing OS-9 module CRC to outfile");
}

unsigned int write_s1(FILE *ifd, FILE *ofd, unsigned bsize, unsigned address)
{
	unsigned char buffer[SREC_LINESIZE];
	unsigned int i, ssize, lines = 0;
	unsigned char csum;

	while (bsize > 0) {
		if (bsize > sizeof(buffer))
			ssize = sizeof(buffer);
		else
			ssize = bsize;

		if ((ssize = fread(buffer, 1, ssize, ifd)) <= 0)
			fatal("Error reading segment from executable");

		fprintf(ofd, "S1%02X%04X", ssize + 3, address);
		csum = ssize + 3 +
		       ((address & 0xff00) >> 8) + (address & 0x00ff);
		address += ssize;
		for (i = 0; i < ssize; i++) {
			csum += buffer[i];
			fprintf(ofd, "%02X", buffer[i]);
		}
		csum ^= 0xff;
		fprintf(ofd, "%02X\n", csum);

		bsize -= ssize;
		lines++;
	}
	return lines;
}

void srec_output(FILE *ifd, FILE *ofd, struct exec header)
{
	unsigned int i, len, s1count;
	unsigned char csum;

	if (!textaddr)
		textaddr = textstart;
	if (!textaddr)
		warning("SREC load address is zero");
	if (!dataaddr)
		dataaddr = datastart;
	if (!dataaddr)
		warning("SREC data load address is zero");

	csum = len = strlen(objname);
	fprintf(ofd, "S0%02X0000", len+3);
	for (i = 0; i < len; i++) {
		fprintf(ofd, "%02X", objname[i]);
	}
	csum ^= 0xff;
	fprintf(ofd, "%02X\n", csum);

	if (fseek(ifd, A_TEXTPOS(header), 0) < 0)
		fatal("Cannot seek to start of text");

	s1count = write_s1(ifd, ofd, header.a_text, textaddr);

	if (fseek(ifd, A_DATAPOS(header), 0) < 0)
		fatal("Cannot seek to start of data");

	s1count += write_s1(ifd, ofd, header.a_data, dataaddr);

	fprintf(ofd, "S503%04X", s1count);
	csum  = 3 + ((s1count & 0xff00) >> 8) + (s1count & 0x00ff);
	csum ^= 0xff;
	fprintf(ofd, "%02X\n", csum);

	fprintf(ofd, "S903%04X", header.a_entry);
	csum  = 3 + ((header.a_entry & 0xff00) >> 8) + (header.a_entry & 0x00ff);
	csum ^= 0xff;
	fprintf(ofd, "%02X\n", csum);
}

void write_ihex(FILE *ifd, FILE *ofd, unsigned bsize, unsigned address)
{
	unsigned char buffer[IHEX_LINESIZE];
	unsigned int i, ssize;
	unsigned char csum;

	while (bsize > 0) {
		if (bsize > sizeof(buffer))
			ssize = sizeof(buffer);
		else
			ssize = bsize;

		if ((ssize = fread(buffer, 1, ssize, ifd)) <= 0)
			fatal("Error reading segment from executable");

		fprintf(ofd, ":%02X%04X00", ssize, address);
		csum = ssize +
		       ((address & 0xff00) >> 8) + (address & 0x00ff);
		address += ssize;
		for (i = 0; i < ssize; i++) {
			csum += buffer[i];
			fprintf(ofd, "%02X", buffer[i]);
		}
		csum = (csum ^ 0xff) + 1;
		fprintf(ofd, "%02X\r\n", csum);

		bsize -= ssize;
	}
}

void ihex_output(FILE *ifd, FILE *ofd, struct exec header)
{
	unsigned char csum;

	if (!textaddr)
		textaddr = textstart;
	if (!textaddr)
		warning("IHEX load address is zero");
	if (!dataaddr)
		dataaddr = datastart;
	if (!dataaddr)
		warning("IHEX data load address is zero");

	if (fseek(ifd, A_TEXTPOS(header), 0) < 0)
		fatal("Cannot seek to start of text");

	write_ihex(ifd, ofd, header.a_text, textaddr);

	if (fseek(ifd, A_DATAPOS(header), 0) < 0)
		fatal("Cannot seek to start of data");

	write_ihex(ifd, ofd, header.a_data, dataaddr);

	fprintf(ofd, ":00%04X01", header.a_entry);
	csum = 1 + ((header.a_entry & 0xff00) >> 8) + (header.a_entry & 0x00ff);
	csum = (csum ^ 0xff) + 1;
	fprintf(ofd, "%02X\n", csum);
}

void write_cas(FILE *ifd, FILE *ofd, unsigned bsize)
{
	unsigned char buffer[CAS_BLOCKSIZE];
	unsigned int i;
	unsigned char csum, ssize;

	while (bsize > 0) {
		if (bsize > sizeof(buffer))
			ssize = sizeof(buffer);
		else
			ssize = bsize;

		if ((ssize = fread(buffer, 1, ssize, ifd)) <= 0)
			fatal("Error reading segment from executable");

		fprintf(ofd, "%c%c%c%c", 0x55, 0x3c, 0x01, ssize);
		if (fwrite(buffer, 1, ssize, ofd) != ssize)
			fatal("Error writing cassette data block to outfile");
		csum = 1 + ssize;
		for (i = 0; i < ssize; i++) {
			csum += buffer[i];
		}
		fprintf(ofd, "%c%c", csum, 0x55);

		bsize -= ssize;
	}
}

void cas_output(FILE *ifd, FILE *ofd, struct exec header)
{
	int i, namelen;
	uint8_t csum, blockdata[21];

	if (!textaddr)
		textaddr = textstart;
	if (!textaddr)
		warning("Color BASIC executable load address is zero");

	if ((header.a_flags & A_SEP) &&
	    (dataaddr || datastart != textend))
		fatal("Color BASIC does not support split text/data");

	blockdata[0] = 0x55;

	for (i = 0; i < CAS_LEADERSIZE; i++)
		fwrite(blockdata, 1, 1, ofd);

	blockdata[1] = 0x3c;
	blockdata[2] = 0;
	blockdata[3] = sizeof(blockdata) - 6;

	namelen = strlen(objname);
	memcpy(&blockdata[4], objname, namelen <= 8 ? namelen : 8);
	if (namelen < 8)
		memset(&blockdata[4+namelen], ' ', 8 - namelen);

	blockdata[12] = 2;
	blockdata[13] = 0;
	blockdata[14] = 0;
	blockdata[15] = (header.a_entry & 0xff00) >> 8;
	blockdata[16] =  header.a_entry & 0x00ff;
	blockdata[17] = (textaddr & 0xff00) >> 8;
	blockdata[18] =  textaddr & 0x00ff;

	csum = 0;
	for (i = 3 ; i < 19; i++)
		csum += blockdata[i];

	blockdata[19] = csum;
	blockdata[20] = 0x55;

	if (fwrite(blockdata, 1, sizeof(blockdata), ofd) != sizeof(blockdata))
		fatal("Error writing Color BASIC name block to outfile");

	for (i = 0; i < CAS_LEADERSIZE; i++)
		fwrite(blockdata, 1, 1, ofd);

	if (fseek(ifd, A_TEXTPOS(header), 0) < 0)
		fatal("Cannot seek to start of text");

	write_cas(ifd, ofd, header.a_text);

	if (fseek(ifd, A_DATAPOS(header), 0) < 0)
		fatal("Cannot seek to start of data");

	write_cas(ifd, ofd, header.a_data);

	blockdata[2] = 0xff;
	blockdata[3] = 0;
	blockdata[4] = 0xff;
	blockdata[5] = 0x55;

	if (fwrite(blockdata, 1, 6, ofd) != 6)
		fatal("Error writing Color BASIC EOF block to outfile");
}

unsigned int wav_samples = 0;

uint8_t wavhdr[] = {
	0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00,
	0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,
	0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
	0xc0, 0xa8, 0x00, 0x00, 0xc0, 0xa8, 0x00, 0x00,
	0x01, 0x00, 0x08, 0x00, 0x64, 0x61, 0x74, 0x61,
	0x00, 0x00, 0x00, 0x00
};

uint8_t wavdat[36] = {
	0x80, 0x90, 0xa8, 0xb8, 0xc8, 0xd8,
	0xe8, 0xf0, 0xf8, 0xf8, 0xf8, 0xf0,
	0xe8, 0xd8, 0xc8, 0xb8, 0xa8, 0x90,
	0x78, 0x68, 0x50, 0x40, 0x30, 0x20,
	0x10, 0x08, 0x00, 0x00, 0x00, 0x08,
	0x10, 0x20, 0x30, 0x40, 0x50, 0x68,
};

void write_wavdat(FILE *ofd, uint8_t val)
{
	int i, j, steps;

	for (i = 0; i < 8; val >>= 1, i++) {
		if (val & 1)
			steps = 2;
		else
			steps = 1;

		for (j = 0; j < sizeof(wavdat); j += steps)
			if (fwrite(&wavdat[j], 1, 1, ofd) != 1)
				fatal("Error writing WAV data to outfile");

		wav_samples += sizeof(wavdat) / steps;
	}
}

void write_wav(FILE *ifd, FILE *ofd, unsigned bsize)
{
	unsigned char buffer[CAS_BLOCKSIZE];
	unsigned int i;
	unsigned char csum, ssize;

	while (bsize > 0) {
		if (bsize > sizeof(buffer))
			ssize = sizeof(buffer);
		else
			ssize = bsize;

		if ((ssize = fread(buffer, 1, ssize, ifd)) <= 0)
			fatal("Error reading segment from executable");

		write_wavdat(ofd, 0x55);
		write_wavdat(ofd, 0x3c);
		write_wavdat(ofd, 0x01);
		write_wavdat(ofd, ssize);
		csum = 1 + ssize;
		for (i = 0; i < ssize; i++) {
			write_wavdat(ofd, buffer[i]);
			csum += buffer[i];
		}
		write_wavdat(ofd, csum);
		write_wavdat(ofd, 0x55);

		bsize -= ssize;
	}
}

void write_wavgap(FILE *ofd)
{
        int i;
        unsigned char c = 0x80;

        for (i = 0; i < WAV_HZ / 2; i++)
                if (fwrite(&c, 1, 1, ofd) != 1)
                        fatal("Error writing WAV data (silence) to outfile");

        wav_samples += WAV_HZ / 2;
}

void wav_output(FILE *ifd, FILE *ofd, struct exec header)
{
	int i, namelen;
	uint8_t csum, blockdata[21];

	if (!textaddr)
		textaddr = textstart;
	if (!textaddr)
		warning("Color BASIC executable load address is zero");

	if ((header.a_flags & A_SEP) &&
	    (dataaddr || datastart != textend))
		fatal("Color BASIC does not support split text/data");

	if (fwrite(wavhdr, 1, sizeof(wavhdr), ofd) != sizeof(wavhdr))
		fatal("Error writing WAV header to outfile");

	write_wavgap(ofd);

	for (i = 0; i < CAS_LEADERSIZE; i++)
		write_wavdat(ofd, 0x55);

	blockdata[0] = 0x55;
	blockdata[1] = 0x3c;
	blockdata[2] = 0;
	blockdata[3] = sizeof(blockdata) - 6;

	namelen = strlen(objname);
	memcpy(&blockdata[4], objname, namelen <= 8 ? namelen : 8);
	if (namelen < 8)
		memset(&blockdata[4+namelen], ' ', 8 - namelen);

	blockdata[12] = 2;
	blockdata[13] = 0;
	blockdata[14] = 0;
	blockdata[15] = (header.a_entry & 0xff00) >> 8;
	blockdata[16] =  header.a_entry & 0x00ff;
	blockdata[17] = (textaddr & 0xff00) >> 8;
	blockdata[18] =  textaddr & 0x00ff;

	csum = 0;
	for (i = 3 ; i < 19; i++)
		csum += blockdata[i];

	blockdata[19] = csum;
	blockdata[20] = 0x55;

	for (i = 0; i < sizeof(blockdata); i++)
		write_wavdat(ofd, blockdata[i]);

	write_wavgap(ofd);

	for (i = 0; i < CAS_LEADERSIZE; i++)
		write_wavdat(ofd, 0x55);

	if (fseek(ifd, A_TEXTPOS(header), 0) < 0)
		fatal("Cannot seek to start of text");

	write_wav(ifd, ofd, header.a_text);

	if (fseek(ifd, A_DATAPOS(header), 0) < 0)
		fatal("Cannot seek to start of data");

	write_wav(ifd, ofd, header.a_data);

	blockdata[2] = 0xff;
	blockdata[3] = 0;
	blockdata[4] = 0xff;
	blockdata[5] = 0x55;

	for (i = 0; i < 6; i++)
		write_wavdat(ofd, blockdata[i]);

	if (fseek(ofd, 40, 0) < 0)
		fatal("Cannot seek to start of WAV data in outfile");

	blockdata[0] =  (wav_samples        & 0xff);
	blockdata[1] = ((wav_samples >> 8)  & 0xff);
	blockdata[2] = ((wav_samples >> 16) & 0xff);
	blockdata[3] = ((wav_samples >> 24) & 0xff);

	if (fwrite(blockdata, 1, 4, ofd) != 4)
		fatal("Error writing size info to WAV header in outfile");

	wav_samples += 36;

	if (fseek(ofd, 4, 0) < 0)
		fatal("Cannot seek to start of WAV data in outfile");

	blockdata[0] =  (wav_samples        & 0xff);
	blockdata[1] = ((wav_samples >> 8)  & 0xff);
	blockdata[2] = ((wav_samples >> 16) & 0xff);
	blockdata[3] = ((wav_samples >> 24) & 0xff);

	if (fwrite(blockdata, 1, 4, ofd) != 4)
		fatal("Error writing size info to WAV header in outfile");
}

int main(int argc, char *argv[])
{
	FILE *ifd, *ofd;
	int i, opt;
	struct exec header;
	enum output_formats outform = BINARY;

	progname = argv[0];
	while ((opt = getopt(argc, argv, "O:T:D:d:n:v:rp")) != -1) {
		switch(opt) {
		case 'O':
			if (!strncmp(optarg, "binary", 3))
				outform = BINARY;
			else if(!strncmp(optarg, "decb", 4))
				outform = DECB_BIN;
			else if(!strncmp(optarg, "ddos", 4))
				outform = DDOS_BIN;
			else if (!strncmp(optarg, "os9", 3))
				outform = OS9_MODULE;
			else if (!strncmp(optarg, "srec", 3))
				outform = SREC;
			else if (!strncmp(optarg, "ihex", 3))
				outform = IHEX;
			else if (!strncmp(optarg, "cas", 3))
				outform = CAS;
			else if (!strncmp(optarg, "wav", 3))
				outform = WAV;
			else
				fatal("Unknown output format!\n");
			break;
		case 'T':
			errno = 0;
			textaddr = strtol(optarg, NULL, 0);
			if (errno)
				fatal("Bad text address value");
			break;
		case 'D':
			errno = 0;
			dataaddr = strtol(optarg, NULL, 0);
			if (errno)
				fatal("Bad data address value");
			break;
		case 'd':
			errno = 0;
			datasize = strtol(optarg, NULL, 0);
			if (errno)
				fatal("Bad data size value");
			break;
		case 'n':
			objname = optarg;
			break;
		case 'v':
			errno = 0;
			objversion = strtol(optarg, NULL, 0);
			if (errno)
				fatal("Bad version value");
			break;
		case 'r':
			reentrant = 1;
			break;
		case 'p':
			padoutput = 1;
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (optind > argc - 2) {
		usage();
		exit(EXIT_FAILURE);
	}

	ifd = fopen(argv[optind], "r");
	if (ifd == 0)
		fatal("Cannot open input file");

	if (fread(&header, A_MINHDR, 1, ifd) != 1)
		fatal("Incomplete executable header");

	if (BADMAG(header))
		fatal("Input file has bad magic number");

	exec_header_adjust(&header);

	match_syms(ifd, header);

	ofd = fopen(argv[optind + 1], "w");
	if (ofd == 0)
		fatal("Cannot open output file");

	if (!objname) {
		objname = argv[optind + 1];
		for (i = 0; i < strlen(objname); i++)
			objname[i] = toupper(objname[i]);
	}

	switch(outform) {
	case BINARY:
		raw_output(ifd, ofd, header);
		break;
	case DECB_BIN:
		decb_output(ifd, ofd, header);
		break;
	case DDOS_BIN:
		ddos_output(ifd, ofd, header);
		break;
	case OS9_MODULE:
		os9_output(ifd, ofd, header);
		break;
	case SREC:
		srec_output(ifd, ofd, header);
		break;
	case IHEX:
		ihex_output(ifd, ofd, header);
		break;
	case CAS:
		cas_output(ifd, ofd, header);
		break;
	case WAV:
		wav_output(ifd, ofd, header);
		break;
	default:
		fatal("Unknown output format");
	};

	fclose(ifd);
	fclose(ofd);

	exit(EXIT_SUCCESS);
}
