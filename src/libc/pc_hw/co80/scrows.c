/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include "sc.h"

int
ScreenRows(void)
{
  return _farpeekb(dossel, 0x484) + 1;
}
