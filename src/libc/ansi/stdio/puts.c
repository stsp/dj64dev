/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include "libc/file.h"

int
puts(const char *s)
{
  int c;

  while ((c = *s++))
    putchar(c);
  if (!__is_text_file(stdout))
    putchar('\r');
  return putchar('\n');
}
