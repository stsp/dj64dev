# Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details
TOP=..

SRC += brk.c
SRC += c1args.c
SRC += c1loadef.c
SRC += c1pglob.c
SRC += crt1.c
#SRC += mcount.c
SRC += memhandl.c
SRC += rfinfo.c
SRC += dfinfo.c
AS_SRC += crt0.S

include $(TOP)/../../Makefile.conf
include $(srcdir)/$(TOP)/../makefile.inc

crt0.o : crt0.S exit16.ah sbrk16.ah
