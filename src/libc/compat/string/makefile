# Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details
TOP=../..

#AS_SRC += ffs.S
SRC += memccpy.c
SRC += memicmp.c
SRC += stpcpy.c
SRC += stpncpy.c
SRC += strcasec.c
SRC += strdup.c
SRC += stricmp.c
SRC += strlcat.c
SRC += strlcpy.c
SRC += strlwr.c
SRC += strncase.c
SRC += strnicmp.c
SRC += strsep.c
SRC += strupr.c

include $(TOP)/../../Makefile.conf
include $(srcdir)/$(TOP)/../makefile.inc

stpcpy.o:	EXTRA_CFLAGS += -fno-builtin-stpcpy
stpncpy.o:	EXTRA_CFLAGS += -fno-builtin-stpncpy
strdup.o:	EXTRA_CFLAGS += -fno-builtin-strdup
