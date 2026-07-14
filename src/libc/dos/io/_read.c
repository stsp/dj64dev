/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <go32.h>
#include <dpmi.h>
#include <io.h>
#include <sys/fsext.h>
#include <libc/fsexthlp.h>
#include <libc/dosio.h>

int
_read(int handle, void* buffer, unsigned count)
{
  size_t j, k;
  int ngot;
  __dpmi_regs r = {0};

  __FSEXT_Function *func = __FSEXT_get_function(handle);
  if (func)
  {
    int rv;
    if (__FSEXT_func_wrapper(func, __FSEXT_read, &rv, handle, buffer, count))
      return rv;
  }

  ngot = 0;
  do {
    j = (count <= __tb_size) ? count : __tb_size;
    r.x.ax = 0x3f00;
    r.x.bx = handle;
    r.x.cx = j;
    r.x.dx = __tb & 15;
    r.x.ds = __tb / 16;
    __dpmi_int(0x21, &r);
    if(r.x.flags & 1)
    {
      errno = __doserr_to_errno(r.x.ax);
      return -1;
    }
    count -= j;
    k = r.x.ax;
    ngot += k;
    if (k)
      dosmemget(__tb, k, buffer);
    buffer = (void *)((uintptr_t)buffer + k);
  } while(count && j == k);	/* if not == on DOS then no more */
  return ngot;
}
