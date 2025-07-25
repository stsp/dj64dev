/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <dpmi.h>
#include <go32.h>
#include <crt0.h>
#include <dos.h>
#include <libc/farptrgs.h>
#include <libc/dosio.h>

char *
__getcwd(char *buf, size_t size)
{
  char *bp;
  __dpmi_regs r = {0};
  size_t needed_length, buf_off;
  int c;
  unsigned use_lfn = _USE_LFN;
  int preserve_case = _preserve_fncase();
  char *name_start;

  if (!size)
  {
    errno = EINVAL;
    return 0;
  }
  if (!buf)
  {
    buf = (char *)malloc(size);
    if (!buf)
    {
      errno = ENOMEM;
      return 0;
    }
  }

  /* make sure we don't overrun the TB */
  if (size > __tb_size)
    size = __tb_size;

  /* get the path into the transfer buffer at least */
  if (use_lfn)
  {
    r.x.flags = 1;  /* Always set CF before calling a 0x71NN function. */
    r.x.ax = 0x7147;
  }
  else
    r.h.ah = 0x47;
do_getcwd:
  r.h.dl = 0;
  r.x.si = __tb_offset;
  r.x.ds = __tb_segment;
  __dpmi_int(0x21, &r);

  if (r.x.ax == 0x7100)
  {
    /*  Never assume that the complete LFN API is implemented,
        so check that AX != 0x7100.  E.G.: MSDOS 6.22 and DOSLFN 0.40.
        If not supported fall back on SFN API 0x47.  */
    use_lfn = 0;
    r.h.ah = 0x47;
    goto do_getcwd;
  }
  else if (r.x.flags & 1)
  {
    /* current drive may be invalid (it happens) */
    errno = __doserr_to_errno(r.x.ax);
    return 0;
  }

  /* path is ASCIIZ.  Scan it, filling in buf, watching for
     end of buffer. */
  _farsetsel(_dos_ds);
  needed_length = 0;
  buf_off = 3;
  if (_os_trueversion == 0x532 && use_lfn && _farnspeekb(__tb) == 0)
  {
    /* Root path under WinNT/2K/XP with lfn (may be silent failure).
       If the short name equivalent of the current path is greater than
       64 characters, Windows 2000 and XP do not return the correct long
       path name - they return the root directory instead without any
       failure code.  We check for this bug and try and fix the path. */

    r.x.ax = 0x7160;
    r.x.cx = 0x8002;	/* Get Long Path Name, using subst drive basis */
    r.x.es = __tb_segment;
    r.x.si = __tb_offset + FILENAME_MAX;
    r.x.di = __tb_offset;

    _farnspokew(__tb + FILENAME_MAX, '.');	/* Null terminated */
    __dpmi_int(0x21, &r);

    if (r.x.flags & 1)
      _farnspokeb(__tb, 0);		/* Put back as root if failure */
    else
      buf_off = 0;
  }
  while ((c = _farnspeekb(__tb+needed_length)))
  {
    if (needed_length + buf_off >= size)
    {
      errno = ERANGE;
      return 0;
    }
    buf[buf_off + needed_length] = c;
    needed_length++;
  }
  buf[needed_length + buf_off] = 0;

  /* switch FOO\BAR to foo/bar, downcase where appropriate */
  buf[1] = ':';
  buf[2] = '/';
  for (bp = buf+3, name_start = bp - 1; *name_start; bp++)
  {
    char long_name[FILENAME_MAX];

    if (*bp == '\\')
      *bp = '/';
    if (!preserve_case && (*bp == '/' || *bp == '\0'))
    {
      memcpy(long_name, name_start+1, bp - name_start - 1);
      long_name[bp - name_start - 1] = '\0';
      if (_is_DOS83(long_name))
      {
	while (++name_start < bp)
	  if (*name_start >= 'A' && *name_start <= 'Z')
	    *name_start += 'a' - 'A';
      }
      else
	name_start = bp;
    }
    else if (*bp == '\0')
      break;
  }

  /* get current drive */
  r.h.ah = 0x19;
  __dpmi_int(0x21, &r);

  buf[0] = r.h.al + (r.h.al < 26 ? 'a' : 'A');

  return buf;
}

#ifdef __APPLE__
char * getcwd(char *buf, size_t size)
{
  return __getcwd(buf, size);
}
#else
char * __attribute__((alias("__getcwd"))) getcwd(char *buf, size_t size);
#endif

#ifdef TEST

int main(int argc, char** argv)
{
  if(argc > 1)
  {
    if(chdir(argv[1]) == -1)
      perror("Change dir failed");
    else
      printf("Change dir to %s\n",argv[1]);
  }

  printf (getcwd ((char *)0, FILENAME_MAX + 10));
  return 0;
}

#endif
