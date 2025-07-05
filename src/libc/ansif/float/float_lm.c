/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <libc/ieee.h>

#ifdef HAVE___FLOAT80
_longdouble_union_t __dj_long_double_min = { .ldt = { 0x00000000, 0x80000000U, 0x0001, 0x0 } };
#else
_longdouble_union_t __dj_long_double_min = { .ldt = { 0x00000000, 0x00000, 0x001, 0x0 } };
#endif
