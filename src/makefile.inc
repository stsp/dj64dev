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
export CROSS_NM
export UCTAGS
export URTAGS

export prefix

.SUFFIXES:

CPPFLAGS += -DDJ64 -D_IN_DJ64 -D__DJGPP__ -nostdinc

CPP = $(CC) -E
CPP32 = $(CC32) -E
XGCC = $(CC) $(DJ64_CFLAGS)
XGCC32 = $(CC32) $(DJ32_CFLAGS) $(CROSS_CFLAGS)
XCPP = $(CPP) -I . -I $(srcdir)/$(TOP)/../../include $(CPPFLAGS) -DUSE64=1 \
  -imacros $(TOP)/../../config.hh -iquote $(srcdir)
XCPP32 = $(CPP32) -I . -I $(srcdir)/$(TOP)/../../include $(CPPFLAGS) -DDJ32 -DUSE64=0 \
  -imacros $(TOP)/../../config.hh -iquote $(srcdir)
CROSS_ASCPP = $(3) -x assembler-with-cpp $(CPPFLAGS) -DUSE64=$(2) $(1) | \
  $(CROSS_AS) $(CROSS_ASFLAGS) -I $(srcdir) -
XASCPP = $(call CROSS_ASCPP, -I $(srcdir)/$(TOP)/../../include $(1),$(2),$(3))

# Have to process with intermediate file, because with a single pipe
# the partially preprocessed (before cpp hit an error) input is fed
# to the compiler, and may actually successfully compile. :(
# This is regardless of pipefail bash mode:
# https://stackoverflow.com/questions/73162200/bash-set-eo-pipefail-not-immediately-exiting
%.C : %.c
	$(XCPP) -CC -g0 -MD -MT $@ -MF $@.d $< | \
	  sed -E -e 's/^\*\/#/\*\/\n#/' -e 's/^\*\/ *;/\*\/\n;\n/' >$@
%.o : %.C
	$(XGCC) -xc $< -c $(EXTRA_CFLAGS) -o $@
%.C32 : %.c
	$(XCPP32) -CC -g0 -MD -MT $@ -MF $@.d $< | \
	  sed -E -e 's/^\*\/#/\*\/\n#/' -e 's/^\*\/ *;/\*\/\n;\n/' >$@
%.O : %.C32
	$(XGCC32) -xc $< -c $(EXTRA_CFLAGS) -o $@
%.o : %.S
	$(call XASCPP,$<,1,$(CPP)) -o $@
%.O : %.S
	$(call XASCPP,$<,0,$(CPP32)) -o $@

# By default, explicit targets are not intermediate. This happens when
# .d files are included, which may trigger the second rebuild due to
# building all previously-intermediate files that were erased.
# Note: neither .INTERMEDIATE nor .SECONDARY do work with patterns,
# but .PRECIOUS does. So here we list them explicitly. That forces
# them back to an intermediate status, even is presence of .d.
# Note that .SECONDARY also includes .INTERMEDIATE. I list them both
# only for completeness.
GENERATED = $(SRC:.c=.C) $(SRC64:.c=.C) $(SRC:.c=.C32) $(SRC32:.c=.C32)
.INTERMEDIATE: $(GENERATED)
# for debug-src builders
.SECONDARY: $(GENERATED)

LIB=$(TOP)/../../lib
BIN=$(TOP)/../../bin
INC=$(TOP)/../../include
C = $(LIB)/crt0.o
L = $(LIB)/libc.a

OBJS := $(addsuffix .o, $(basename $(SRC) $(SRC64)))
OBJS32 := $(addsuffix .O, $(basename $(SRC) $(SRC32)))
AS_OBJS64 := $(addsuffix .o, $(basename $(AS_SRC) $(AS_SRC64)))
AS_OBJS32 := $(addsuffix .O, $(basename $(AS_SRC) $(AS_SRC32)))

NOP = @echo

ifeq ($(USE64),1)
TARG64 = $(OBJS) $(AS_OBJS64)
endif
ifeq ($(USE32),1)
TARG32 = $(OBJS32) $(AS_OBJS32)
endif
all :: $(TARG64) $(TARG32)
	$(NOP)

$(LIB) :
	mkdir $@

ifeq ($(USE64),1)
MK64 = makefile.oh makefile.o64
endif
ifeq ($(USE32),1)
MK32 = makefile.h32 makefile.o32
endif
all :: $(MK64) $(MK32)
makefile.oh: makefile
	echo $(addprefix \&/,$(OBJS)) >$@
makefile.h32: makefile
	echo $(addprefix \&/,$(OBJS32)) >$@
makefile.o64: makefile
	echo $(addprefix \&/,$(AS_OBJS64)) >$@
makefile.o32: makefile
	echo $(addprefix \&/,$(AS_OBJS32)) >$@

clean ::
	-$(RM) *.o *.O *.d *.C *.C32 *.i makefile.oh makefile.oho
	-$(RM) makefile.h32 makefile.o32 makefile.o64

DEPS := $(wildcard *.d)
ifneq ($(DEPS),)
include $(DEPS)
endif
