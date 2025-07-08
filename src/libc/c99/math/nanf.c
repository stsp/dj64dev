/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float
nanf(const char *tagp)
{
  float ret = NAN;
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
      ret = strtof(buf, NULL);
  }

  return ret;
}
