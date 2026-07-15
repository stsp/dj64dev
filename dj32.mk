# find the suitable cross-assembler
DJ64AS = $(CROSS_PREFIX)as
DJ64ASFLAGS = --32 --defsym _DJ32=1 $(DJASFLAGS)
XSTRIP = $(CROSS_PREFIX)strip --strip-debug
CC = $(CROSS_PREFIX)gcc
XLD = $(CROSS_PREFIX)gcc
XLD_IMB = -Ttext-segment

ifeq ($(CROSS_PREFIX),)
CROSS_PREFIX := i686-linux-gnu-
ifeq ($(shell $(DJ64AS) --version 2>/dev/null),)
CROSS_PREFIX := i686-unknown-linux-gnu-
ifeq ($(shell $(DJ64AS) --version 2>/dev/null),)
CROSS_PREFIX := i586-suse-linux-
ifeq ($(shell $(DJ64AS) --version 2>/dev/null),)
CROSS_PREFIX := x86_64-linux-gnu-
ifeq ($(shell $(DJ64AS) --version 2>/dev/null),)
CROSS_PREFIX :=
endif
endif
endif
endif
else ifeq ($(shell $(DJ64AS) --version 2>/dev/null),)
# CROSS_PREFIX already set
$(error invalid CROSS_PREFIX)
endif
XCFLAGS = -m32
XLDFLAGS = -m32

# Override external AS as termux sets it to aarch64-linux-android-clang
# omitting -c. Note that plain as also doesn't work for termux.
AS = $(CC) -x assembler-with-cpp -c

DJ64CFLAGS := $(DJCFLAGS) $(shell pkg-config --cflags dj32) $(XCFLAGS)
ifneq ($(.SHELLSTATUS),0)
$(error dj32-dev not installed)
endif
XCPPFLAGS = $(DJCPPFLAGS) $(shell pkg-config --variable=xcppflags dj32)
DJ64ASCPPFLAGS = $(DJASCPPFLAGS) $(shell pkg-config --variable=cppflags dj32)

LOADADDR = 0x08148000
# static
DJ64_XLDFLAGS += -f 0x40
# ELF
DJ64_XLDFLAGS += -f 0x80 -V 8
DJ64_LINKARGS += -T 1 -D 5
XELF = tmp.elf

ifeq ($(filter clean install,$(MAKECMDGOALS)),)
ifneq ($(PDHDR),)
HASH := \#
ifneq ($(shell grep "ASMCFUNC" $(PDHDR) | grep -cv "$(HASH)define"),0)
PLT_O = plt.o
endif
endif
endif
GLOB_ASM ?= $(wildcard glob_asm.h)

DJ64_XLDFLAGS += -f 0x6000
S := $(shell pkg-config --static --libs dj32)
ifneq ($(.SHELLSTATUS),0)
$(error dj32-dev not installed)
endif
XLDFLAGS += $(S)
$(XELF): $(AS_OBJECTS) $(PLT_O) $(OBJECTS)
	$(XLD) $^ $(DJLDFLAGS) $(XLDFLAGS) -o $@
DJ64_XLIB = $(XELF)

%.o: %.c
	$(CC) $(DJ64CFLAGS) $(XCPPFLAGS) -I. -o $@ -c $<
SHELL := /usr/bin/env bash
%.o: %.S
	set -o pipefail; $(CPP) -x assembler-with-cpp $(DJ64ASCPPFLAGS) $< | \
	    $(DJ64AS) $(DJ64ASFLAGS) -o $@ -
plt.o: plt.inc $(GLOB_ASM)
	set -o pipefail; echo "#include <dj64/plt.S.inc>" | \
	    $(CPP) -x assembler-with-cpp -I. $(DJ64ASCPPFLAGS) - | \
	    $(DJ64AS) $(DJ64ASFLAGS) -o $@ -
thunks_c.o: thunk_c1.h thunk_c32.h thunk_calls.h
thunks_p.o: thunk_p1.h thunk_p32.h thunk_asms.h plt_asmc.h

ifneq ($(PDHDR),)
ifneq ($(GLOB_ASM),)
$(OBJECTS): glob_asmdefs.h
endif
# hook in thunk-gen - make sure to not do that before defining `all:` target
TGMK = $(shell pkg-config --variable=makeinc thunk_gen)
ifeq ($(wildcard $(TGMK)),)
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(error thunk_gen not installed)
endif
else # TGMK
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(shell pkg-config --atleast-version=1.2 thunk_gen)
ifneq ($(.SHELLSTATUS),0)
$(error thunk_gen is too old, 1.2 is needed)
endif
endif # clean
TFLAGS = -a 4 -p 4
include $(TGMK)
endif # TGMK
endif # PDHDR

LNK_VER = $(lastword $(shell djlink -v 2>/dev/null))
ifeq ($(LNK_VER),)
$(warning djlink missing or too old)
else
LINK = djlink
endif

clean_dj32:
	$(RM) $(OBJECTS) $(AS_OBJECTS) plt.o plt.inc *.tmp
	$(RM) thunk_*.h plt_asmc.h glob_asmdefs.h
	$(RM) $(XELF) host.elf
