/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2004 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
#include <libc/ieee.h>

#ifdef HAVE___FLOAT80
_longdouble_union_t __dj_huge_vall = { .ldt = { 0x00000000, 0x80000000, 0x7fff, 0x0 } };
#else
_longdouble_union_t __dj_huge_vall = { .ldt = { 0x00000, 0x00000, 0x7ff, 0x0 } };
#endif