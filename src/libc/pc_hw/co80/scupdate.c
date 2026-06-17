/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <stdint.h>
#include "sc.h"

void
ScreenUpdate(const void *_virtual_screen)
{
  fmemcpy1(DP(dossel, co80), _virtual_screen, ScreenRows() * ScreenCols() * 2);
}
