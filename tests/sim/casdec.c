/*
 * casdec - decode an MC-10 cassette file (.cas bytes, or .wav audio) for
 * the regression tests
 *
 * Reads the blocks that the MC-10 ROM's CLOADM reads, checks them, and
 * writes the program they hold as S-records on standard output: the data
 * at the name block's load address, and its exec address as the start
 * address.  A summary goes to standard error.  The tests run the result
 * on the simulator, so that objcopy01's cassette output is checked by
 * loading it as the MC-10 would, not just compared byte by byte.
 *
 * It is written from the cassette format, not from objcopy01:
 *
 * - The audio is one cycle per bit, least significant bit first: a cycle
 *   of 2400 Hz is a 1 and a cycle of 1200 Hz a 0.  Each cycle is measured
 *   between rising zero crossings, using the WAV file's own sample rate
 *   and size (8-bit unsigned or 16-bit signed PCM).
 * - Each block follows a $55 byte (after a leader of them, where there
 *   was a gap) and a $3C sync byte: a block
 *   type ($00 name, $01 data, $FF end of file), a length, the data, and
 *   a checksum, the sum of the type, length and data bytes.
 * - The name block holds an 8-character name, the file type (2 for a
 *   machine language program), an ASCII flag, a gap flag, and the exec
 *   and load addresses (high byte first).
 *
 * usage: casdec [-w] file   (-w: the file is audio, in WAV format)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *progname = "casdec";

static void fail(const char *msg)
{
	fprintf(stderr, "%s: %s\n", progname, msg);
	exit(1);
}

/* the decoded bytes, with the positions where sync was found */
static unsigned char *bytes;
static long nbytes, maxbytes;
static long *syncs;
static long nsyncs;

static void addbyte(int b)
{
	if (nbytes == maxbytes) {
		maxbytes = maxbytes ? 2 * maxbytes : 65536;
		bytes = realloc(bytes, maxbytes);
		syncs = realloc(syncs, maxbytes * sizeof *syncs);
		if (!bytes || !syncs)
			fail("out of memory");
	}
	bytes[nbytes++] = b;
}

/* .cas: the bytes are the file; sync is each $3C that follows a $55 */
static void read_cas(FILE *f)
{
	int c, prev = -1;

	while ((c = getc(f)) != EOF) {
		addbyte(c);
		if (c == 0x3C && prev == 0x55 && nsyncs < maxbytes)
			syncs[nsyncs++] = nbytes;	/* the block starts here */
		prev = c;
	}
}

static long le(const unsigned char *p, int n)
{
	long v = 0;

	while (n--)
		v = (v << 8) | p[n];
	return v;
}

/* .wav: measure each cycle, turn it into a bit, and find the bytes */
static void read_wav(FILE *f)
{
	unsigned char hdr[12], chunk[8], fmt[16];
	long rate = 0, size, period = 0;
	int bits = 0, channels = 0, prev = 0, sample, synced = 0, nbits = 0;
	unsigned shift = 0, byte = 0;

	if (fread(hdr, 1, 12, f) != 12 || memcmp(hdr, "RIFF", 4)
	    || memcmp(hdr + 8, "WAVE", 4))
		fail("not a WAV file");
	for (;;) {		/* find the fmt and data chunks */
		if (fread(chunk, 1, 8, f) != 8)
			fail("no data in the WAV file");
		size = le(chunk + 4, 4);
		if (memcmp(chunk, "fmt ", 4) == 0) {
			if (size < 16 || fread(fmt, 1, 16, f) != 16)
				fail("bad WAV format chunk");
			if (le(fmt, 2) != 1)
				fail("WAV file isn't PCM");
			channels = le(fmt + 2, 2);
			rate = le(fmt + 4, 4);
			bits = le(fmt + 14, 2);
			fseek(f, size - 16 + (size & 1), SEEK_CUR);
		} else if (memcmp(chunk, "data", 4) == 0)
			break;
		else
			fseek(f, size + (size & 1), SEEK_CUR);
	}
	if (channels != 1 || (bits != 8 && bits != 16) || rate < 9600)
		fail("unsupported WAV format (needs mono, 8 or 16 bits)");

	while (size > 0) {
		if (bits == 8) {
			if ((sample = getc(f)) == EOF)
				break;
			sample -= 128;
			size--;
		} else {
			unsigned char s[2];

			if (fread(s, 1, 2, f) != 2)
				break;
			sample = (short)(s[0] | s[1] << 8) / 256;
			size -= 2;
		}
		period++;
		if (prev < 0 && sample >= 0) {	/* a rising zero crossing */
			int bit;

			/* 2400 Hz is a 1, 1200 Hz a 0; anything else
			   (silence, noise) loses sync */
			if (period * 1800 < rate && period * 4800 > rate)
				bit = 1;
			else if (period * 1800 >= rate && period * 800 < rate)
				bit = 0;
			else
				bit = -1;
			period = 0;
			if (bit < 0) {
				synced = 0;
				shift = 0;
			} else if (!synced) {
				/* the leader's $55s, then the $3C sync byte:
				   bits arrive least significant first, so the
				   newer byte is in the top half */
				shift = (shift >> 1) | (bit << 15);
				if (shift == 0x3C55 && nsyncs < maxbytes) {
					synced = 1;
					nbits = byte = 0;
					syncs[nsyncs++] = nbytes;
				}
			} else {
				byte |= bit << nbits;
				if (++nbits == 8) {
					addbyte(byte);
					/* a block can follow the last one
					   with just $55 $3C */
					if (byte == 0x3C && nbytes >= 2
					    && bytes[nbytes - 2] == 0x55
					    && nsyncs < maxbytes)
						syncs[nsyncs++] = nbytes;
					nbits = byte = 0;
				}
			}
		}
		prev = sample;
	}
}

/* S-records */
static void srec(int type, unsigned addr, const unsigned char *data, int n)
{
	int sum = n + 3 + (addr >> 8) + (addr & 0xFF), i;

	printf("S%d%02X%04X", type, n + 3, addr);
	for (i = 0; i < n; i++) {
		printf("%02X", data[i]);
		sum += data[i];
	}
	printf("%02X\n", ~sum & 0xFF);
}

int main(int argc, char **argv)
{
	unsigned char name[9], *prog = 0;
	unsigned load = 0, exec = 0, len = 0;
	int wav = 0, i, type = -1, ascii = -1, gap = -1, ndata = 0, done = 0;
	long s, next = 0;	/* syncs before next are inside a block */
	FILE *f;

	if (argc == 3 && strcmp(argv[1], "-w") == 0)
		wav = 1;
	else if (argc != 2)
		fail("usage: casdec [-w] file");
	if (!(f = fopen(argv[argc - 1], "rb")))
		fail("cannot open the file");
	addbyte(0);		/* allocate */
	nbytes = 0;
	if (wav)
		read_wav(f);
	else
		read_cas(f);
	fclose(f);

	name[0] = 0;
	for (s = 0; s < nsyncs && !done; s++) {
		long p = syncs[s];
		int btype, blen, sum;

		if (p < next)
			continue;
		if (p + 2 > nbytes)
			fail("a block is cut short");
		btype = bytes[p];
		blen = bytes[p + 1];
		if (p + 2 + blen + 1 > nbytes)
			fail("a block is cut short");
		for (sum = btype + blen, i = 0; i < blen; i++)
			sum += bytes[p + 2 + i];
		if ((sum & 0xFF) != bytes[p + 2 + blen])
			fail("block checksum error");
		next = p + 2 + blen + 1;
		switch (btype) {
		case 0x00:	/* name block */
			if (blen != 15 || type >= 0)
				fail("bad name block");
			memcpy(name, bytes + p + 2, 8);
			name[8] = 0;
			type = bytes[p + 10];
			ascii = bytes[p + 11];
			gap = bytes[p + 12];
			exec = bytes[p + 13] << 8 | bytes[p + 14];
			load = bytes[p + 15] << 8 | bytes[p + 16];
			break;
		case 0x01:	/* data block */
			if (type < 0)
				fail("data before the name block");
			prog = realloc(prog, len + blen + 1);
			if (!prog)
				fail("out of memory");
			memcpy(prog + len, bytes + p + 2, blen);
			len += blen;
			ndata++;
			break;
		case 0xFF:	/* end of file */
			if (blen != 0)
				fail("bad end of file block");
			done = 1;
			break;
		default:
			fail("unknown block type");
		}
	}
	if (!done)
		fail("no end of file block");

	fprintf(stderr, "name \"%s\", type %d, %s, gap flag %d, load %04X, "
		"exec %04X, %u bytes in %d data blocks\n", name, type,
		ascii ? "ASCII" : "binary", gap, load, exec, len, ndata);
	srec(0, 0, name, strlen((char *)name));
	for (i = 0; (unsigned)i < len; i += 16)
		srec(1, load + i, prog + i, len - i < 16 ? len - i : 16);
	srec(9, exec, 0, 0);
	return 0;
}
