# Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details
# -*- makefile -*-

include $(TOP)/../../Makefile.conf

SHELL := bash -o pipefail

vpath %.ah $(srcdir)

# for mkproto.sh
export CROSS_LD
export UCTAGS
export URTAGS

export prefix

.SUFFIXES:

CPPFLAGS += -DDJ64 -D_IN_DJ64 -D__DJGPP__ -nostdinc

# Moved setting of $(GCC{L}_OPT) to top of file
#
XGCC = $(CC) $(DJ64_CFLAGS)
XGCC32 = $(CC32) $(CROSS_CFLAGS) $(DJ32_CFLAGS)
XCPP = $(CPP) -I . -I $(srcdir)/$(TOP)/../../include $(CPPFLAGS) -DUSE64=1 \
  -imacros $(TOP)/../../config.hh -iquote $(srcdir)
XCPP32 = $(CPP) -I . -I $(srcdir)/$(TOP)/../../include $(CPPFLAGS) -DDJ32 -DUSE64=0 \
  -imacros $(TOP)/../../config.hh -iquote $(srcdir)
CROSS_ASCPP = $(CPP) -x assembler-with-cpp $(1) | \
  $(CROSS_AS) $(CROSS_ASFLAGS) -I $(srcdir) -
XASCPP = $(call CROSS_ASCPP, -I $(srcdir)/$(TOP)/../../include $(1))

# Have to process with intermediate file, because with a single pipe
# the partially preprocessed (before cpp hit an error) input is fed
# to the compiler, and may actually successfully compile. :(
# This is regardless of pipefail bash mode:
# https://stackoverflow.com/questions/73162200/bash-set-eo-pipefail-not-immediately-exiting
%.C : %.c
	$(XCPP) -CC -g0 -MD $< | \
	  sed -E -e 's/^\*\/#/\*\/\n#/' -e 's/^\*\/ *;/\*\/\n;\n/' >$@
%.o : %.C
	$(XGCC) -xc $< -c $(EXTRA_CFLAGS) -o $@
%.C32 : %.c
	$(XCPP32) -CC -g0 -MD $< | \
	  sed -E -e 's/^\*\/#/\*\/\n#/' -e 's/^\*\/ *;/\*\/\n;\n/' >$@
%.O : %.C32
	$(XGCC32) -xc $< -c $(EXTRA_CFLAGS) -o $@
%.o : %.S
	$(call XASCPP,$<) -o $@
%.o : %.s
	$(call XASCPP,$<) -o $@

LIB=$(TOP)/../../lib
BIN=$(TOP)/../../bin
INC=$(TOP)/../../include
C = $(LIB)/crt0.o
L = $(LIB)/libc.a

OBJS := $(addsuffix .o, $(basename $(SRC) $(SRC64)))
OBJS32 := $(addsuffix .O, $(basename $(SRC) $(SRC32)))
AS_OBJS := $(addsuffix .o, $(basename $(AS_SRC)))
AS_OBJS64 := $(addsuffix .o, $(basename $(AS_SRC64)))
AS_OBJS32 := $(addsuffix .o, $(basename $(AS_SRC32)))

NOP = @echo

.PRECIOUS: %.exe %.out %.err

all :: $(OBJS) $(EXTRA_OBJS) $(EXTRA_FILES) $(AS_OBJS) $(OBJS32) $(AS_OBJS32) $(AS_OBJS64)
	$(NOP)

$(LIB) :
	mkdir $@

ifneq ($(MAKEFILE_LIB),1)
all :: makefile.oh makefile.oho makefile.h32 makefile.o64 makefile.o32
makefile.oh: makefile
	echo $(addprefix \&/,$(OBJS)) >$@
makefile.h32: makefile
	echo $(addprefix \&/,$(OBJS32)) >$@
makefile.oho: makefile
	echo $(addprefix \&/,$(AS_OBJS)) >$@
makefile.o64: makefile
	echo $(addprefix \&/,$(AS_OBJS64)) >$@
makefile.o32: makefile
	echo $(addprefix \&/,$(AS_OBJS32)) >$@
endif

clean ::
	-$(RM) *.o *.O *.d *.C *.C32 *.i makefile.oh makefile.oho
	-$(RM) makefile.h32 makefile.o32 makefile.o64

DEPS := $(wildcard *.d)
ifneq ($(DEPS),)
include $(DEPS)
endif
