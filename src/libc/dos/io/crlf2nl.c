/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <io.h>

int
crlf2nl(char *buf, int len)
{
  char *bp = buf;
  int i=0;
  while (len--)
  {
    if (*bp != 13)
    {
      *buf++ = *bp;
      i++;
    }
    bp++;
  }
  return i;
}
