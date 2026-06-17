/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <stdint.h>
#include "sc.h"

void
ScreenRetrieve(void *_virtual_screen)
{
  fmemcpy2(_virtual_screen, DP(dossel, co80), ScreenRows() * ScreenCols() * 2);
}

