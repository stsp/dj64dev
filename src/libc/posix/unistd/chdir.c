/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <libc/symlink.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <go32.h>
#include <dpmi.h>
#include <stdio.h>
#include <limits.h>
#include <libc/dosio.h>
#include <libc/farptrgs.h>

int
__chdir (const char *mydirname)
{
  __dpmi_regs r = {0};
  int drv_no = -1;
  char real_name[FILENAME_MAX];
  char path[FILENAME_MAX];

  if (!__solve_symlinks(mydirname, real_name))
     return -1;
  _fixpath(real_name, path);

  if (path[0] == 0)
  {
    errno = ENOENT;
    return -1;
  }

  _put_path(path);

  /* _put_path performs some magic conversions of file names, so
     the path in the transfer buffer can include a drive even though
     MYDIRNAME doesn't seem to.  */
  if (_farpeekb(_dos_ds, __tb + 1) == ':')
    drv_no = (_farpeekb(_dos_ds, __tb) & 0x1f) - 1;

  if (drv_no != -1)	/* Must do this first for Windows 2000 */
  {
    /* Change current drive.  The directory change below checks it. */
    r.h.ah = 0x0e;
    r.h.dl = drv_no;
    __dpmi_int(0x21, &r);
  }

  if (drv_no == -1 || _farpeekb(_dos_ds, __tb + 2) != 0)
  {
    if (_USE_LFN)
    {
      r.x.flags = 1;  /* Always set CF before calling a 0x71NN function. */
      r.x.ax = 0x713b;
    }
    else
      r.h.ah = 0x3b;
do_chdir:
    r.x.dx = __tb_offset;
    r.x.ds = __tb_segment;
    __dpmi_int(0x21, &r);

    if (r.x.ax == 0x7100)
    {
      /*  Never assume that the complete LFN API is implemented,
          so check that AX != 0x7100.  E.G.: MSDOS 6.22 and DOSLFN 0.40.
          If not supported fall back on SFN API 0c3B.  */
      r.h.ah = 0x3b;
      goto do_chdir;
    }
    else if (r.x.flags & 1)
    {
      errno = __doserr_to_errno(r.x.ax);
      return -1;
    }
  }

  return 0;
}

#ifdef __APPLE__
int chdir(const char *mydirname)
{
  return __chdir(mydirname);
}
#else
int __attribute__((alias("__chdir"))) chdir(const char *mydirname);
#endif
