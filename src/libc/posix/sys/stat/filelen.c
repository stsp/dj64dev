/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/* This is file FILELEN.C */
/*
 * Copyright (c) 1994 Eli Zaretskii <eliz@is.elta.co.il>
 *
 * This software may be used freely so long as this copyright notice is
 * left intact.  There is no warranty on this software.
 *
 */

#include <libc/stubs.h>
#include <errno.h>
#include <dpmi.h>
#include <libc/dosio.h>
#include <go32.h>
#include <libc/farptrgs.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>


long
filelength(int fhandle)
{
  __dpmi_regs    regs = {0};
  unsigned short fpos_high, fpos_low;
  long           retval;

  /* Use the LFN API when available to get the file length.  Under NT/Win2K/XP
     we don't do this optimization to avoid a bug on pre-opened handles.  The
     seeks below this section (as always used in 2.03) makes later lseek/read
     combos work properly.  This assumes this routine is called from fstat()
     before we get the magic number or other things that do both seeks and
     reads. */
  if (_USE_LFN && (fhandle != 0 || _os_trueversion != 0x532))
  {
    regs.x.flags = 1;  /* Always set CF before calling a 0x71NN function. */
    regs.x.ax = 0x71A6;
    regs.x.bx = fhandle;
    regs.x.ds = __tb >> 4;
    regs.x.dx = 0;
    __dpmi_int(0x21, &regs);

    /*  It is always necessary to test if LFN function
        has been implemented because the assumption has
        been proven false that a driver will set the CF
        if the LFN function has not been implemented.
        E.g.: all DOSLFN drivers do not implement
        0x71A6 and DOSLFN 0.40e does not set CF
        making MSDOS 6.22 fail.  If FreeDOS 1.0 is
        used, the same LFN driver sets the CF.
        If the ax register contains 0x7100 then the
        corresponding LFN function is not implemented.
        If the 0x71A6 function is not supported fall back
        on 0x42NN.  */
    if (!(regs.x.flags & 1) && (regs.x.ax != 0x7100))
    {
      /* Offset 0x24 contains the low 32-bits of the file size.
         Offset 0x20 contains the high 32-bits.  */
      retval = _farpeekl(_dos_ds, __tb + 0x24);

      if ((_farpeekl(_dos_ds, __tb + 0x20) != 0) || (retval == -1))
      {
        errno = EOVERFLOW;
        return -1L;
      }
      return retval;
    }
  }

  /* Remember the current file position, so we can return there
     later.  */
  regs.x.ax = 0x4201;      /* set pointer from current position */
  regs.x.bx = fhandle;
  regs.x.cx = regs.x.dx = 0; /* move 0 bytes (i.e., stay put) */
  __dpmi_int(0x21, &regs);
  if (regs.x.flags & 1)
  {
    errno = __doserr_to_errno(regs.x.ax);
    return -1L;
  }
  fpos_high = regs.x.dx;   /* save current position */
  fpos_low  = regs.x.ax;

  regs.x.cx = regs.x.dx = 0;
  regs.x.ax = 0x4202;      /* set pointer 0 bytes from the end of file */
  __dpmi_int(0x21, &regs);
  if (regs.x.flags & 1)
  {
    errno = __doserr_to_errno(regs.x.ax);
    return -1L;
  }

  /* The absolute byte offset returned in DX:AX is the file size. */
  retval = ( (long)regs.x.dx << 16 ) + regs.x.ax;

  /* Leave things as we have found them. */
  regs.x.ax = 0x4200;      /* set pointer from the beginning of file */
  regs.x.cx = fpos_high;
  regs.x.dx = fpos_low;
  __dpmi_int(0x21, &regs);
  if (regs.x.flags & 1)
  {
    errno = __doserr_to_errno(regs.x.ax);
    return -1L;
  }

  return retval;
}

#ifdef __APPLE__
long __filelength(int fhandle);
long __filelength(int fhandle)
{
  return filelength(fhandle);
}
#else
long __attribute__((alias("filelength"))) __filelength(int fhandle);
#endif
