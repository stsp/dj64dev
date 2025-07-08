/* Copyright (C) 2013 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long double
nanl(const char *tagp)
{
  long double ret = NAN;
  char buf[256];
  int s;

//  if (tagp)
  {
    /*
     * If we can't fit NAN(<tagp>) in the buffer, just return NAN.
     * It seems better to return a plain NAN than some possibly bogus NAN.
     */
    s = snprintf(buf, sizeof(buf), "NAN(%s)", tagp);
    if (s < (ssize_t) sizeof(buf))
      ret = strtold(buf, NULL);
  }

  return ret;
}
