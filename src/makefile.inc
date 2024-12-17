# Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details
# -*- makefile -*-

include $(TOP)/../../Makefile.conf

SHELL := bash -o pipefail

# for mkproto.sh
export CROSS_LD

export prefix

.SUFFIXES: .o .i .c .cc .s

CPPFLAGS += -DDJ64 -D_IN_DJ64 -D__DJGPP__ -nostdinc

# Moved setting of $(GCC{L}_OPT) to top of file
#
XGCC = $(CC) @$(TOP)/../gcc.opt $(CFLAGS)
XCPP = $(CPP) -I . -I $(TOP)/../../include $(CPPFLAGS)
CROSS_ASCPP = $(CPP) -x assembler-with-cpp $(1) | $(CROSS_AS) --32 -
XASCPP = $(call CROSS_ASCPP, -I $(TOP)/../../include $(ASFLAGS) $(1))

# Have to process with intermediate file, because with a single pipe
# the partially preprocessed (before cpp hit an error) input is fed
# to the compiler, and may actually successfully compile. :(
# This is regardless of pipefail bash mode:
# https://stackoverflow.com/questions/73162200/bash-set-eo-pipefail-not-immediately-exiting
%.o : %.c
	$(XCPP) -CC -g0 -MD $< | \
	  sed -E -e 's/^\*\/#/\*\/\n#/' -e 's/^\*\/ *;/\*\/\n;\n/' >$@.c
	$(XGCC) -xc $@.c -c $(EXTRA_CFLAGS) -o $@
	$(RM) $@.c
%.o : %.S
	$(call XASCPP,$<) -o $@
%.o : %.s
	$(call XASCPP,$<) -x assembler-with-cpp -o $@

LIB=$(TOP)/../../lib
BIN=$(TOP)/../../bin
INC=$(TOP)/../../include
C = $(LIB)/crt0.o
L = $(LIB)/libc.a

OBJS := $(addsuffix .o, $(basename $(SRC)))
AS_OBJS := $(addsuffix .o, $(basename $(AS_SRC)))
EXES := $(addprefix $(BIN)/,$(addsuffix .exe, $(basename $(SRC))))

NOP = @echo

.PRECIOUS: %.exe %.out %.err

all :: $(OBJS) $(EXTRA_OBJS) $(EXTRA_FILES) $(AS_OBJS)
	$(NOP)

$(LIB) :
	mkdir $@

ifneq ($(MAKEFILE_LIB),1)
all :: makefile.oh makefile.oho
makefile.oh: makefile
	echo $(addprefix \&/,$(OBJS)) >$@
makefile.oho: makefile
	echo $(addprefix \&/,$(AS_OBJS)) >$@
endif

clean ::
	-$(RM) *.o *.d *.i makefile.oh makefile.oho

DEPS := $(wildcard *.d)
ifneq ($(DEPS),)
include $(DEPS)
endif
