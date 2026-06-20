srcdir = ..
vpath %.c $(srcdir)
vpath %.S $(srcdir)

SOURCES := hello.c
SOURCES := $(addprefix $(srcdir)/,$(SOURCES))
TGT = hello

# the below 4 vars must be named exactly like here
DJCFLAGS = -Wall -ggdb3
DJCPPFLAGS = -I. -I$(srcdir)
OBJECTS = $(notdir $(SOURCES:.c=.o))

all: $(TGT)

clean: clean_dj32
	$(RM) $(TGT)

# display info about the built executable
info: $(TGT)
	djstubify -i $<

run: $(TGT)
	dosemu -dumb -l $(realpath $<)

# hook in dj64 - make sure to not do that before defining `all:` target
DJMK = $(shell pkg-config --silence-errors --variable=makeinc dj32)
ifeq ($(wildcard $(DJMK)),)
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(error dj32 not installed)
endif
clean_dj32:
else
include $(DJMK)
endif

# dj64 sets `DJ64_XOBJS`, `DJ64_XLIB`, `DJ64_XLDFLAGS`
$(TGT): $(DJ64_XLIB)
	cp $< $@
