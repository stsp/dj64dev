TOP = ../../..
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
LIBN = libdjstub64.so.0.2
LIB = $(LIBDIR)/$(LIBN)

all: $(LIBDIR)/libdjstub64.so

clean:
	$(RM) *.o *.d $(LIBDIR)/libdjstub64.so*

$(LIBDIR):
	mkdir $@

$(LIB): $(OBJECTS) libdjstub.version | $(LIBDIR)
	$(CC) $(LDFLAGS) -shared -Wl,-Bsymbolic -Wl,-soname=$(LIBN) \
		-Wl,--version-script=libdjstub.version -o $@ $(OBJECTS)

$(LIBDIR)/libdjstub64.so: $(LIB)
	ln -sf $(LIBN) $@

-include $(wildcard *.d)
