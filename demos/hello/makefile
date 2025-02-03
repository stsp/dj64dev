SOURCES = hello.c
TGT = hello

# the below 4 vars must be named exactly like here
CFLAGS = -Wall -ggdb3
OBJECTS = $(SOURCES:.c=.o)

all: $(TGT)

static:
	$(MAKE) DJ64STATIC=1

clean: clean_dj64
	$(RM) $(TGT)

# display info about the built executable
info: $(TGT)
	djstubify -i $<

run: $(TGT)
	dosemu -dumb -l $(realpath $<)

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
ifeq ($(DJ64STATIC),1)
$(error this demo does not support static linking)
endif
$(TGT): $(DJ64_XLIB)
	cp $< $@

djgpp/%:
	$(MAKE) -C djgpp $*
