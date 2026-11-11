
LIBDIR	=$(PREFIX)
CFLAGS	=-O
LDFLAGS	=

VERSION	:=$(shell cat ../VERSION)
DEFS	=-DREL_OUTPUT -DVERSION=\"$(VERSION)\"

OBJS= dumps.o io.o ld.o readobj.o table.o typeconv.o linksyms.o mkar.o \
      script.o srec.o \
      writex86.o

all: ld01 objdump01 objcopy01

ld01: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

objdump01: objdump86.c m6801ops.c m6801ops.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) objdump86.c m6801ops.c -o $@

objcopy01: objcopy.c srec.c srec.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) objcopy.c srec.c -o $@

install: ld01 objdump01 objcopy01
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 ld01 $(DESTDIR)$(LIBDIR)
	install -m 755 objdump01 $(DESTDIR)$(LIBDIR)
	ln -f $(DESTDIR)$(LIBDIR)/objdump01 $(DESTDIR)$(LIBDIR)/nm01
	ln -f $(DESTDIR)$(LIBDIR)/objdump01 $(DESTDIR)$(LIBDIR)/size01
	install -m 755 objcopy01 $(DESTDIR)$(LIBDIR)

clean realclean clobber:
	rm -f *.o ld01 objdump01 objcopy01

io.o: ../VERSION

$(OBJS): align.h byteord.h config.h const.h globvar.h obj.h \
	 syshead.h type.h x86_aout.h script.h srec.h

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEFS) -c $< -o $@
