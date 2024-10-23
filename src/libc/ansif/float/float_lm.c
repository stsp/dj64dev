/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <libc/ieee.h>

_longdouble_union_t __dj_long_double_min = { .ldt = { 0x00000000, 0x80000000U, 0x0001, 0x0 } };
