/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <stdint.h>
#include "sc.h"

void
ScreenUpdateLine(const void *_virtual_screen_line, int _row)
{
  fmemcpy1(DP(dossel, co80 + ScreenCols() * 2 * _row),
	   _virtual_screen_line, ScreenCols() * 2);
}
