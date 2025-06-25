/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <crt0.h>

char
_preserve_fncase (void)
{
  char *ep;

  if (!_USE_LFN)
    return 0;
  return ((_crt0_startup_flags & _CRT0_FLAG_PRESERVE_FILENAME_CASE)
	  || ((ep = getenv ("FNCASE")) && tolower ((unsigned char)*ep) == 'y'));
}
