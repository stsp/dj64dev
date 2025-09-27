# Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details
TOP = ..
include $(TOP)/../../Makefile.conf
include $(srcdir)/$(TOP)/../makefile.inc

STUBH = $(srcdir)/$(TOP)/../../include/libc/stubs.h

all :: makefil2
	$(MAKE) -f makefil2

makefil2 : mkstubs.exe  $(STUBH) makefile
	./mkstubs.exe $(STUBH)

mkstubs.exe : $(TOP)/../mkstubs.c
	$(CC_FOR_BUILD) $< -o mkstubs.exe

clean ::
	@-$(RM) *.S makefil2 mkstubs.exe
