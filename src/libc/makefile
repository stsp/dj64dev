# Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details
# Note that makefile.oi, makefile.rf and makefile.sub are normally
# *generated* but they are included in the source distribution to ease
# cross-compiling.

TOP=.

LIBNAME=c

include $(TOP)/../../Makefile.conf

LIB = $(TOP)/../../lib

all:
	../makemake.exe
	$(MAKE) -f makefile.sub
	../makemake.exe -2
	$(MAKE) -C dj64 final

.PHONY: all info tex texinfo doc libc2.tex

clean:
	$(TOP)/../makemake.exe
	$(MAKE) -f makefile.sub SUBARGS=clean
	$(RM) makefile.oi makefile.rf makefile.rfo
	$(RM) makefile.sub
