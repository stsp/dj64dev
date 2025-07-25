/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <go32.h>
#include <dpmi.h>
#include <dir.h>
#include <fcntl.h>
#include <libc/dosio.h>

int
findnext(struct ffblk *ffblk)
{
  __dpmi_regs r = {0};

  if (ffblk == 0)
  {
    errno = EACCES;
    return -1;
  }

  if (_USE_LFN)
  {
    struct ffblklfn ffblk32;

    /* si = 1 indicates DOS style dates, 0 means Win32 type dates.
       DOS style dates are broken in some Win95 betas, build for either.
       Release works with DOS date, it's faster, so use it. */
    #define USEDOSDATE 1
    #if USEDOSDATE == 1
      #define _Win32_to_DOS (long)
    #else
      extern long _Win32_to_DOS(long long WinTime);
    #endif

    /* Clear result area to avoid WinXP bug not setting ending 0 into filename
       in case it contains a Unicode character greater than 255. */
    memset(&ffblk32, 0, sizeof(ffblk32));
    dosmemput(&ffblk32, sizeof(ffblk32), __tb);

    r.x.flags = 1;  /* Always set CF before calling a 0x71NN function. */
    r.x.ax = 0x714f;
    r.x.bx = ffblk->lfn_handle;
    if (!r.x.bx)
    {
      errno = ENMFILE;
      return 1;
    }
    r.x.di = __tb_offset;
    r.x.es = __tb_segment;
    r.x.si = USEDOSDATE;

    __dpmi_int(0x21, &r);
    if (!(r.x.flags & 1) && (r.x.ax != 0x7100))
    {
      /*  Never assume that the complete LFN API is implemented,
          so check that AX != 0x7100.  E.G.: MSDOS 6.22 and DOSLFN 0.40.
          If not supported fall back on SFN API 0x4F.  */

      unsigned long t1;
      /* Recover results */
      dosmemget(__tb, sizeof(struct ffblklfn), &ffblk32);

      ffblk->ff_attrib = (char)ffblk32.fd_attrib;
      t1 = _Win32_to_DOS(ffblk32.fd_mtime);
      ffblk->ff_ftime = t1;
      ffblk->ff_fdate = t1 >> 16;
      ffblk->ff_fsize = ffblk32.fd_size;
      strcpy(ffblk->ff_name, ffblk32.fd_longname);
      t1 = _Win32_to_DOS(ffblk32.fd_ctime);
      ffblk->lfn_ctime = t1;
      ffblk->lfn_cdate = t1 >> 16;
      t1 = _Win32_to_DOS(ffblk32.fd_atime);
      ffblk->lfn_atime = t1;
      ffblk->lfn_adate = t1 >> 16;

      return 0;
    }
    errno = __doserr_to_errno(r.x.ax);
    if (errno == ENMFILE)         /* call FindClose */
    {
      ffblk->lfn_handle = 0;
      r.x.flags |= 1;  /* Always set CF before calling a 0x71NN function. */
      r.x.ax = 0x71a1;
      __dpmi_int(0x21, &r);
      if (r.x.flags & 1)
      {
        errno = __doserr_to_errno(r.x.ax);
        return -1;
      }
      return 1;
    }
    return -1;
  }
  else
  {
    #define _sizeof_dos_ffblk 44
    /* The 43 character ff block must be put to the DTA, make the call, then
       recover the ff block. */

    r.x.dx = __tb_offset;
    r.x.ds = __tb_segment;
    r.h.ah = 0x1a;
    __dpmi_int(0x21, &r);

    dosmemput(ffblk, sizeof(struct ffblk), __tb);

    r.h.ah = 0x4f;
    __dpmi_int(0x21, &r);
    if (r.x.flags & 1)
    {
      errno = __doserr_to_errno(r.x.ax);
      return -1;
    }

    /* Recover results */
    dosmemget(__tb, _sizeof_dos_ffblk, ffblk);
    return 0;
  }
}

#ifdef __APPLE__
int __findnext(struct ffblk *ffblk);
int __findnext(struct ffblk *ffblk)
{
  return findnext(ffblk);
}
#else
int __attribute__((alias("findnext"))) __findnext(struct ffblk *ffblk);
#endif
