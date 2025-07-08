/* Copyright (C) 2005 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define UNKNOWN_ERROR_STR "Unknown error: "

int
strerror_r(int errnum, char *strerrbuf, size_t buflen)
{
  char ebuf[ sizeof(UNKNOWN_ERROR_STR "-2147483648") ]; /* -2147483648 is
							       INT_MIN. */
  const char *p;
  int length;


  if (errnum >= 0 && errnum < __sys_nerr)
  {
    p = __sys_errlist[errnum];
    length = strlen(p);
  }
  else
  {
    length = sprintf(ebuf, "%s%d", UNKNOWN_ERROR_STR, errnum);
    p = ebuf;
  }

  if (length < 0 || buflen < (size_t)length+1)
  {
    return ERANGE;
  }

  strcpy(strerrbuf, p);
  return 0;
}
