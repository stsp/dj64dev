/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include "sc.h"

void
ScreenPutString(const char *_ch, int _attr, int _x, int _y)
{
  int scptr;
  if ((_x < 0) || (_y < 0))
    return;
  if ((_x >= ScreenCols()) || (_y >= ScreenRows()))
    return;
  _attr = (_attr & 0xff) << 8;
  scptr = co80 + (_x+_y*ScreenCols())*2;
  _farsetsel(dossel);
  while (*_ch)
  {
    _farnspokew(scptr, (*_ch & 0xff)|_attr);
    _ch++;
    scptr += 2;
  }
}
