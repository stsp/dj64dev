/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <libc/ieee.h>

#ifdef HAVE___FLOAT80
_longdouble_union_t __dj_long_double_max = { .ldt = { 0xffffffffU, 0xffffffffU, 0x7ffe, 0x0 } };
#else
_longdouble_union_t __dj_long_double_max = { .ldt = { 0xffffffffU, 0xfffff, 0x7fe, 0x0 } };
#endif
