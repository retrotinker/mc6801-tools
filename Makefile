.PHONY: all clean install install-man check clean-sim

PREFIX=/usr/local/m6801-tools
MANDIR=/usr/local/share/man

# demo is not listed: it needs the tools installed and in PATH
DIRS=ar as ld

# the MC6801 simulator that the tests run programs on, and a decoder for
# MC-10 cassette files (not installed)
SIM=tests/sim/sim6801
CASDEC=tests/sim/casdec

all clean install: $(DIRS)
	for i in $(DIRS) ;\
	do \
		$(MAKE) -C $$i DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) $@ || exit 1;\
	done

clean: clean-sim

# doc is not in DIRS, since building the manual needs pandoc and LaTeX
install: install-man

install-man:
	$(MAKE) -C doc DESTDIR=$(DESTDIR) MANDIR=$(MANDIR) install-man

# regression tests: compare outputs with the references in tests/ref
check: all $(SIM) $(CASDEC)
	tests/run.sh

$(SIM): tests/sim/sim6801.c ld/m6801ops.c ld/m6801ops.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ tests/sim/sim6801.c ld/m6801ops.c

$(CASDEC): tests/sim/casdec.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ tests/sim/casdec.c

clean-sim:
	rm -f $(SIM) $(CASDEC)
