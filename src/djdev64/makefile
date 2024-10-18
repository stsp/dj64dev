TOP = ../..
PREFIX ?= /usr/local
PKG_CONFIG ?= pkg-config
SOURCES = djdev64.c elf.c djexec.c
OBJECTS = $(SOURCES:.c=.o)
LIBELF := $(shell $(PKG_CONFIG) --silence-errors --libs libelf)
ifeq ($(LIBELF),)
$(error libelf not installed)
endif
CFLAGS += -iquote include -fpic -ggdb3 -Wall -Og -MD \
  -DCRT0=\"$(PREFIX)/i386-pc-dj64/lib/crt0.elf\"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

LIBDIR = $(TOP)/lib
LIBN = libdjdev64.so.0.2
LIB = $(LIBDIR)/$(LIBN)

.PHONY: subs

all: $(LIBDIR)/libdjdev64.so subs

subs:
	$(MAKE) -C stub

clean:
	$(MAKE) -C stub clean
	$(RM) *.o $(LIBDIR)/libdjdev64.so*

$(LIBDIR):
	mkdir $@

$(LIB): $(OBJECTS) | $(LIBDIR)
	$(CC) -shared -Wl,-Bsymbolic -Wl,-soname=$(LIBN) -o $@ $^ $(LIBELF)

$(LIBDIR)/libdjdev64.so: $(LIB)
	ln -sf $(LIBN) $@

-include $(wildcard *.d)
