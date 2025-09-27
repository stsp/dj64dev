# Copyright (C) 2013 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2005 DJ Delorie, see COPYING.DJ for details
TOP=../..

SRC += strtok_r.c
SRC += strerr_r.c
SRC += strndup.c
SRC += strnlen.c

include $(TOP)/../../Makefile.conf
include $(srcdir)/$(TOP)/../makefile.inc

strndup.o:	EXTRA_CFLAGS += -fno-builtin-strndup
strnlen.o:	EXTRA_CFLAGS += -fno-builtin-strnlen
