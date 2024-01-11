
LIBDIR	=$(PREFIX)
CFLAGS	=-O -DDATASEGS
LDFLAGS	=

# Will need some of these if you want native executables on non-Linux/i386
# -DDETECTAOUT			# Turn on detection.
# -DV7_A_OUT			# a.out.h is like V7
# -DBSD_A_OUT			# a.out.h is like BSD
# -DSTANDARD_GNU_A_OUT		# a.out.h is like GNU normal.
#
# -DREL_OUTPUT -DBUGCOMPAT	# -r Produces weird *.o files.
#
DEFS	=-DREL_OUTPUT

# An alternative file for a non-standard a.out.h (eg i386 linux on an Alpha)
#
# NATIVE=-DA_OUT_INCL='"a_out_local.h"' 

OBJS= dumps.o io.o ld.o readobj.o table.o typeconv.o linksyms.o mkar.o \
      writex86.o

all: ld01 objdump01 objcopy01

ld01: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

objdump01: objdump86.c
	$(CC) $(LDFLAGS) $< -o $@

objcopy01: objcopy.c
	$(CC) $(LDFLAGS) $< -o $@

install: ld01 objdump01
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 ld01 $(DESTDIR)$(LIBDIR)
	install -m 755 objdump01 $(DESTDIR)$(LIBDIR)
	ln -f $(DESTDIR)$(LIBDIR)/objdump01 $(DESTDIR)$(LIBDIR)/nm01
	ln -f $(DESTDIR)$(LIBDIR)/objdump01 $(DESTDIR)$(LIBDIR)/size01
	install -m 755 objcopy01 $(DESTDIR)$(LIBDIR)

clean realclean clobber:
	rm -f *.o ld01 objchop catimage objdump01 objcopy01

$(OBJS): align.h bindef.h byteord.h config.h const.h globvar.h obj.h \
	 syshead.h type.h x86_aout.h

.c.o:
	$(CC) $(CFLAGS) $(DEFS) -c $< -o $@
