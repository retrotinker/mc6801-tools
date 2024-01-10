.PHONY: all clean

PREFIX=/usr/local/m6801-tools

DIRS=$(shell find * -type d -prune)

all clean install: $(DIRS)
	for i in $^ ;\
	do \
		$(MAKE) -C $$i PREFIX=$(PREFIX) $@;\
	done
