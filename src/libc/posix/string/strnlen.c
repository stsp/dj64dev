/* Copyright (C) 2013 DJ Delorie, see COPYING.DJ for details */
#include <string.h>

size_t
strnlen(const char *_str, size_t _n)
{
  if (*_str == '\0' || _n == 0)
    return 0;
  else
  {
    size_t i;

    for (i = 1; _str[i] && i < _n; i++)
      ;

    return i;
  }
}

