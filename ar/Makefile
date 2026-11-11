# Copyright (c) 1999 Greg Haerr <greg@censoft.com>
# This file is part of the Linux-8086 Development environment and is
# distributed under the GNU General Public License.

LIBDIR	=$(PREFIX)
CFLAGS	=-O
LDFLAGS	=
DEFS	=
OBJS= ar.o

all: ar01

ar01: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

install: ar01
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 ar01 $(DESTDIR)$(LIBDIR)

clean realclean clobber:
	rm -f *.o ar01

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEFS) -c $< -o $@
