/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include "sc.h"

void
ScreenSetCursor(int _row, int _col)
{
  __dpmi_regs r = {0};
  r.h.ah = 2;
  r.h.bh = 0;
  r.h.dh = _row;
  r.h.dl = _col;
  __dpmi_int(0x10, &r);
}
