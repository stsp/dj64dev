/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double
nan(const char *tagp)
{
  double ret = NAN;
  char buf[256];
  ssize_t s;

//  if (tagp)
  {
    /*
     * If we can't fit NAN(<tagp>) in the buffer, just return NAN.
     * It seems better to return a plain NAN than some possibly bogus NAN.
     */
    s = snprintf(buf, sizeof(buf), "NAN(%s)", tagp);
    if (s < (ssize_t) sizeof(buf))
      ret = strtod(buf, NULL);
  }

  return ret;
}
