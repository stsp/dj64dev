/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include "sc.h"

void
ScreenPutChar(int _ch, int _attr, int _x, int _y)
{
  if ((_x < 0) || (_y < 0))
    return;
  if ((_x >= ScreenCols()) || (_y >= ScreenRows()))
    return;
  _ch &= 0xff;
  _attr = (_attr & 0xff) << 8;
  _farpokew(dossel, co80 + (_x+_y*ScreenCols())*2, _ch|_attr);
}
