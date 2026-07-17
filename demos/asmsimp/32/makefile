srcdir = ..
vpath %.c $(srcdir)
vpath %.S $(srcdir)

DJSTRIP = @true
# or use `DJSTRIP = djstrip` for non-debug build

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
	$(RM) -r tmp

# display info about the built executable
info: $(TGT)
	djstubify -i $<

run: $(TGT)
	dosemu -dumb $<

runelf: tmp.elf
	dosemu -dumb -l $< -g 1

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

ifneq ($(CROSS_PREFIX),)
# for djlink on arm
export READELF = $(CROSS_PREFIX)readelf
export OBJCOPY = $(CROSS_PREFIX)objcopy
export STRIP = $(CROSS_PREFIX)strip
endif

# dj64 sets `DJ64_XOBJS`, `DJ64_XLIB`, `DJ64_XLDFLAGS`
$(TGT): $(DJ64_XLIB)
	$(LINK) -d $@.dbg $(DJ64_LINKARGS) $^ -o $@ $(DJ64_XLDFLAGS)
	$(DJSTRIP) $@
