/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include "sc.h"

void
ScreenClear(void)
{
  int i = ScreenRows() * ScreenCols();
  unsigned sp = co80;
  unsigned short a = (ScreenAttrib << 8) | ' ';
  _farsetsel(dossel);
  do {
    _farnspokew(sp, a);
    sp += 2;
  } while (--i);
}
