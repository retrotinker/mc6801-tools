.PHONY: all clean install

PREFIX=/usr/local/m6801-tools

# demo is not listed: it needs the tools installed and in PATH
DIRS=ar as ld

all clean install: $(DIRS)
	for i in $^ ;\
	do \
		$(MAKE) -C $$i DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) $@ || exit 1;\
	done
