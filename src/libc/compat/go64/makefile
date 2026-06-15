# Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details
TOP=../..

include $(TOP)/../../Makefile.conf

ifeq ($(USE64),1)
AS_SRC += pltcall32.S
AS_SRC += pltctrl32.S
AS_SRC += upltinit32.S
SRC += elfexec.c
endif

include $(srcdir)/$(TOP)/../makefile.inc
