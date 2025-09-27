TOP = ../..
include $(TOP)/Makefile.conf
SOURCES = djdev64.c elf.c djexec.c
OBJECTS = $(SOURCES:.c=.o)
CFLAGS += -iquote $(srcdir)/include -fpic -ggdb3 -Wall -Wmissing-prototypes -Og -MD \
  $(LIBELF_CFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -imacros $(TOP)/config.hh -c -o $@ $<

LIBDIR = $(TOP)/lib
OS = $(shell uname -s)
ifeq ($(OS),Darwin)
LIBN = libdjdev64.0.2.dylib
LIBNS = libdjdev64.dylib
SHARED_LDFLAGS = $(LDFLAGS) -dynamiclib -install_name $(libdir)/$(LIBN) -compatibility_version 0.2 -current_version 0.2
else
LIBN = libdjdev64.so.0.2
LIBNS = libdjdev64.so
SHARED_LDFLAGS = $(LDFLAGS) -shared -Wl,-soname=$(LIBN)
endif
LIB = $(LIBDIR)/$(LIBN)

.PHONY: subs

all: $(LIBDIR)/$(LIBNS) subs

subs:
	$(MAKE) -C stub

clean:
	$(MAKE) -C stub clean
	$(RM) *.o *.d $(LIBDIR)/libdjdev64.so* $(LIBDIR)/libdjdev64.*dylib

$(LIBDIR):
	mkdir $@

$(LIB): $(OBJECTS) | $(LIBDIR)
	$(CC) $(SHARED_LDFLAGS) -o $@ $^ \
  $(LIBELF_LIBS)

$(LIBDIR)/$(LIBNS): $(LIB)
	ln -sf $(LIBN) $@

-include $(wildcard *.d)
