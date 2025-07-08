TOP = ../..
include $(TOP)/Makefile.conf
SOURCES = djdev64.c elf.c djexec.c
OBJECTS = $(SOURCES:.c=.o)
CFLAGS += -iquote include -fpic -ggdb3 -Wall -Wmissing-prototypes -Og -MD \
  $(LIBELF_CFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -imacros $(TOP)/config.hh -c -o $@ $<

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
	$(CC) $(LDFLAGS) -shared -Wl,-Bsymbolic -Wl,-soname=$(LIBN) -o $@ $^ \
  $(LIBELF_LIBS)

$(LIBDIR)/libdjdev64.so: $(LIB)
	ln -sf $(LIBN) $@

-include $(wildcard *.d)
