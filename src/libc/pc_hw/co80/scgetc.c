/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include "sc.h"

void
ScreenGetCursor(int *_row, int *_col)
{
  __dpmi_regs r = {0};
  r.h.ah = 3;
  r.h.bh = 0;
  __dpmi_int(0x10, &r);
  *_row = r.h.dh;
  *_col = r.h.dl;
}
