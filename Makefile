.PHONY: all clean install install-man

PREFIX=/usr/local/m6801-tools
MANDIR=/usr/local/share/man

# demo is not listed: it needs the tools installed and in PATH
DIRS=ar as ld

all clean install: $(DIRS)
	for i in $(DIRS) ;\
	do \
		$(MAKE) -C $$i DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) $@ || exit 1;\
	done

# doc is not in DIRS, since building the manual needs pandoc and LaTeX
install: install-man

install-man:
	$(MAKE) -C doc DESTDIR=$(DESTDIR) MANDIR=$(MANDIR) install-man
