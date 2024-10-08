/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
#include <dpmi.h>
#include <io.h>

int
_dos_unlk64(int _fd, long long _offset, long long _length)
{
  __dpmi_regs r = {0};
  r.x.ax = 0x5c01;
  r.x.bx = _fd;
  r.x.cx = _offset >> 16;
  r.x.dx = _offset & 0xffff;
  r.x.si = _length >> 16;
  r.x.di = _length & 0xffff;
  __dpmi_int(0x21, &r);
  if (r.x.flags & 1)
    return r.x.ax;
  return 0;
}
