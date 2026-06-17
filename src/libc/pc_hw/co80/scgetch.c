/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
#include "sc.h"

void
ScreenGetChar(int *_ch, int *_attr, int _x, int _y)
{
  int s;

  if ((_x < 0) || (_y < 0))
    return;
  if ((_x >= ScreenCols()) || (_y >= ScreenRows()))
    return;
  s = _farpeekw(dossel, co80 + (_x+_y*ScreenCols())*2);
  if (_ch)
    *_ch = s & 0xff;
  if (_attr)
    *_attr = (s >> 8) & 0xff;
}
