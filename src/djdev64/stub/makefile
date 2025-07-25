TOP = ../../..
include $(TOP)/Makefile.conf
prefix ?= /usr/local
SOURCES = coff.c elf.c util.c stub.c dpmiwrp.c dos.c
OBJECTS = $(SOURCES:.c=.o)
CFLAGS += -iquote ../include -fpic -ggdb3 -Wall -Og -MD \
  -DCRT0=\"$(prefix)/i386-pc-dj64/lib/crt0.elf\"
SYM_VER = 5

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.version: %.version.in makefile
	sed \
		-e 's!@SYM_VER[@]!$(SYM_VER)!g' \
		$< >$@

LIBDIR = $(TOP)/lib
OS = $(shell uname -s)
ifeq ($(OS),Darwin)
LIBN = libdjstub64.0.2.dylib
LIBNS = libdjstub64.dylib
SHARED_LDFLAGS = $(LDFLAGS) -dynamiclib -install_name $(libdir)/$(LIBN) -compatibility_version 0.2 -current_version 0.2
else
LIBN = libdjstub64.so.0.2
LIBNS = libdjstub64.so
SHARED_LDFLAGS = $(LDFLAGS) -shared -Wl,-soname=$(LIBN) \
	-Wl,--version-script=libdjstub.version
endif
LIB = $(LIBDIR)/$(LIBN)

all: $(LIBDIR)/$(LIBNS)

clean:
	$(RM) *.o *.d $(LIBDIR)/libdjstub64.so* $(LIBDIR)/libdjstub64.*dylib

$(LIBDIR):
	mkdir $@

$(LIB): $(OBJECTS) libdjstub.version | $(LIBDIR)
	$(CC) $(SHARED_LDFLAGS) \
		-o $@ $(OBJECTS)

$(LIBDIR)/$(LIBNS): $(LIB)
	ln -sf $(LIBN) $@

-include $(wildcard *.d)
