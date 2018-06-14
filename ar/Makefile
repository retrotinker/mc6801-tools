# Copyright (c) 1999 Greg Haerr <greg@censoft.com>
# This file is part of the Linux-8086 Development environment and is
# distributed under the GNU General Public License.

LIBDIR	=$(PREFIX)/bin
CFLAGS	=-O
LDFLAGS	=
DEFS	=
OBJS= ar.o alloca.o

all: ar01

ar01: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

install: ar01
	install -d $(LIBDIR)
	install -m 755 ar01 $(LIBDIR)

clean realclean clobber:
	rm -f *.o ar01 rel_aout.h

$(OBJS): rel_aout.h

rel_aout.h:
	test -f rel_aout.h || \
	{ rm -f rel_aout.h ; ln -s ../ld/rel_aout.h . ; } || \
	ln ../ld/rel_aout.h .

.c.o:
	$(CC) $(CFLAGS) $(DEFS) -c $< -o $@
