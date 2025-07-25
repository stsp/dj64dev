/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <go32.h>
#include <dpmi.h>
#include <dir.h>
#include <libc/dosio.h>

int
findfirst(const char *pathname, struct ffblk *ffblk, int attrib)
{
  __dpmi_regs r = {0};
  int pathlen;
  int use_lfn = _USE_LFN;

  if (pathname == 0 || ffblk == 0)
  {
    errno = EACCES;
    return -1;
  }

  /* The low 8 bits are the allowable attributes; the next 8 bits
     are the required attributes. We have no required attributes,
     so mask off bits 8-15. */
  attrib &= 0xff;

  pathlen = strlen(pathname) + 1;

  _put_path(pathname);
  if (use_lfn)
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
    dosmemput(&ffblk32, sizeof(ffblk32), __tb + pathlen);

    r.x.flags = 1;  /* Always set CF before calling a 0x71NN function. */
    r.x.ax = 0x714e;
    r.x.cx = attrib;
    r.x.dx = __tb_offset;
    r.x.ds = __tb_segment;
    r.x.di = __tb_offset + pathlen;
    r.x.es = r.x.ds;
    r.x.si = USEDOSDATE;
    __dpmi_int(0x21, &r);
    if (!(r.x.flags & 1) && (r.x.ax != 0x7100))
    {
      /*  Never assume that the complete LFN API is implemented,
          so check that AX != 0x7100.  E.G.: MSDOS 6.22 and DOSLFN 0.40.
          If not supported fall back on SFN API 0x4E.  */

      unsigned long t1;
      /* Recover results */
      dosmemget(__tb + pathlen, sizeof(struct ffblklfn), &ffblk32);

      ffblk->ff_attrib = (char)ffblk32.fd_attrib;

      t1 = _Win32_to_DOS(ffblk32.fd_mtime);
      ffblk->ff_ftime = t1;
      ffblk->ff_fdate = t1 >> 16;

      ffblk->ff_fsize = ffblk32.fd_size;
      strcpy(ffblk->ff_name, ffblk32.fd_longname);
      strcpy(ffblk->lfn_magic, "LFN32");

      /* If no wildcards, close the handle */
      if (attrib == FA_LABEL ||
          (!strchr(pathname, '*') && !strchr(pathname, '?')))
      {
        r.x.flags |= 1;  /* Always set CF before calling a 0x71NN function. */
        r.x.bx = r.x.ax;
        r.x.ax = 0x71a1;
        __dpmi_int(0x21, &r);
        r.x.ax = 0;
      }

      ffblk->lfn_handle = r.x.ax;
      t1 = _Win32_to_DOS(ffblk32.fd_ctime);
      ffblk->lfn_ctime = t1;
      ffblk->lfn_cdate = t1 >> 16;

      t1 = _Win32_to_DOS(ffblk32.fd_atime);
      ffblk->lfn_atime = t1;
      ffblk->lfn_adate = t1 >> 16;

      return 0;
    }
  }
  else
  {

    #define _sizeof_dos_ffblk 44
    /* There will be a _sizeof_dos_ffblk character return value from findfirst
       in the DTA.  Put the file name before this.  First set the DTA to be
       transfer buffer. */

    r.x.dx = __tb_offset + pathlen;
    r.x.ds = __tb_segment;
    r.h.ah = 0x1a;
    __dpmi_int(0x21, &r);

    r.h.ah = 0x4e;
    r.x.dx = __tb_offset;
    r.x.ds = __tb_segment;
    r.x.cx = attrib;
    __dpmi_int(0x21, &r);
    if (!(r.x.flags & 1))
    {
      /* Recover results */
      dosmemget(__tb+pathlen, _sizeof_dos_ffblk, ffblk);
      return 0;
    }
  }

  errno = __doserr_to_errno(r.x.ax);
  return errno;
}

#ifdef __APPLE__
int __findfirst(const char *pathname, struct ffblk *ffblk, int attrib);
int __findfirst(const char *pathname, struct ffblk *ffblk, int attrib)
{
  return findfirst(pathname, ffblk, attrib);
}
#else
int __attribute__((alias("findfirst")))
__findfirst(const char *pathname, struct ffblk *ffblk, int attrib);
#endif
