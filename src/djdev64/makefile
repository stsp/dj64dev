TOP = ../..
prefix ?= /usr/local
PKG_CONFIG ?= pkg-config
SOURCES = djdev64.c elf.c djexec.c
OBJECTS = $(SOURCES:.c=.o)
LIBELF := $(shell $(PKG_CONFIG) --silence-errors --libs libelf)
ifeq ($(LIBELF),)
$(error libelf not installed)
endif
CFLAGS += -iquote include -fpic -ggdb3 -Wall -Wmissing-prototypes -Og -MD \
  $(shell $(PKG_CONFIG) --cflags libelf)

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
	$(RM) *.o *.d $(LIBDIR)/libdjdev64.so*

$(LIBDIR):
	mkdir $@

$(LIB): $(OBJECTS) | $(LIBDIR)
	$(CC) $(LDFLAGS) -shared -Wl,-Bsymbolic -Wl,-soname=$(LIBN) -o $@ $^ $(LIBELF)

$(LIBDIR)/libdjdev64.so: $(LIB)
	ln -sf $(LIBN) $@

-include $(wildcard *.d)
