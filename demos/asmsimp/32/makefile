srcdir = ..
vpath %.c $(srcdir)
vpath %.S $(srcdir)

STRIP = @true
# or use `STRIP = djstrip` for non-debug build

SOURCES := hello.c thunks_p.c
SOURCES := $(addprefix $(srcdir)/,$(SOURCES))
AS_SOURCES := h.S
AS_SOURCES := $(addprefix $(srcdir)/,$(AS_SOURCES))
TGT = asmsimp.exe

# the below 4 vars must be named exactly like here
DJCFLAGS = -Wall -ggdb3
DJCPPFLAGS = -I. -I$(srcdir)
DJASFLAGS = -I. -I$(srcdir)
OBJECTS = $(notdir $(SOURCES:.c=.o))
AS_OBJECTS = $(notdir $(AS_SOURCES:.S=.o))
PDHDR = $(srcdir)/asm.h

all: $(TGT)

clean: clean_dj32
	$(RM) $(TGT)

# display info about the built executable
info: $(TGT)
	djstubify -i $<

run: $(TGT)
	dosemu -dumb $<

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
	$(LINK) -d $@.dbg $^ -o $@ $(DJ64_XLDFLAGS)
	$(STRIP) $@
