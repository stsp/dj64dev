/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include "sc.h"

int
ScreenCols(void)
{
  return _farpeekw(dossel, 0x44a);
}
