# Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details
# Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details
TOP=../..

SRC += abort.c
SRC += abs.c
SRC += atexit.c
SRC += atof.c
SRC += atoi.c
SRC += atol.c
SRC += atold.c
SRC += atoll.c
SRC += bsearch.c
SRC += div.c
SRC += exit.c
SRC += getenv.c
SRC += labs.c
SRC += ldiv.c
SRC += llabs.c
SRC += lldiv.c
#SRC += nmalign.c
SRC += nmalloc.c
#SRC += nmalldbg.c
SRC += malloc.c
SRC += qsort.c
SRC += rand.c
SRC += strtod.c
SRC += strtol.c
SRC += strtold.c
SRC += strtoll.c
SRC += strtoul.c
SRC += strtoull.c
SRC += system.c

# FIXME: This is really only needed for gcc-5 .
#        Unfortunatelly GCC version is not yet checked
#        here.
nmalloc.o: EXTRA_CFLAGS := -fno-builtin-malloc

include $(TOP)/../../Makefile.conf
include $(srcdir)/$(TOP)/../makefile.inc
