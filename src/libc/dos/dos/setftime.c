/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <dos.h>
#include <libc/dosio.h>
#include <errno.h>
#include <dpmi.h>

int setftime(int handle, struct ftime *ft)
{
  __dpmi_regs r = {0};
  r.h.ah = 0x57;
  r.h.al = 1;
  r.x.bx = handle;
  r.x.cx = ((short *)ft)[0];
  r.x.dx = ((short *)ft)[1];
  __dpmi_int(0x21, &r);
  if (r.x.flags & 1)
  {
    errno = __doserr_to_errno(r.x.ax);
    return -1;
  }
  ((short *)ft)[0] = r.x.cx;
  ((short *)ft)[1] = r.x.dx;
  return 0;
}
