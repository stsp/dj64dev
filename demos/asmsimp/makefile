LINK = djlink
STRIP = @true
# or use `STRIP = djstrip` for non-debug build

SOURCES = hello.c thunks_p.c
AS_SOURCES = h.S
TGT = asmsimp.exe

# the below 4 vars must be named exactly like here
CFLAGS = -Wall -ggdb3
OBJECTS = $(SOURCES:.c=.o)
AS_OBJECTS = $(AS_SOURCES:.S=.o)
PDHDR = asm.h

all: $(TGT)

static:
	$(MAKE) DJ64STATIC=1

clean: clean_dj64
	$(RM) $(TGT)

# display info about the built executable
info: $(TGT)
	djstubify -i $<

run: $(TGT)
	dosemu -dumb $<

# hook in dj64 - make sure to not do that before defining `all:` target
DJMK = $(shell pkg-config --silence-errors --variable=makeinc dj64)
ifeq ($(wildcard $(DJMK)),)
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(error dj64 not installed)
endif
clean_dj64:
else
include $(DJMK)
endif

# dj64 sets `DJ64_XOBJS`, `DJ64_XLIB`, `DJ64_XLDFLAGS`
$(TGT): $(DJ64_XOBJS)
	$(LINK) -d dosemu_$@.dbg $(DJ64_XLIB) -n $@ -o $@ $(DJ64_XLDFLAGS)
	$(STRIP) $@

djgpp/%:
	$(MAKE) -C djgpp $*
