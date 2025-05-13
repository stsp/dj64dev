/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2008 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <stdio.h>		/* For FILENAME_MAX */
#include <stdlib.h>
#include <errno.h>		/* For errno */
#include <string.h>		/* For strlen() */
#include <fcntl.h>		/* For LFN stuff */
#include <go32.h>
#include <dpmi.h>		/* For dpmisim */
#include <crt0.h>		/* For crt0 flags */
#include <dos.h>		/* For Win NT version check */
#include <sys/stat.h>
#include <libc/dosio.h>

#define IS_DRIVE_SPECIFIER(path)  ((((path)[0] == (drive_number + 'A')) || ((path)[0] == (drive_number + 'a'))) && ((path)[1] == ':'))
#define IS_ROOT_DIR(path)         (((path)[2] == '\\') && ((path)[3] == '\0'))


static unsigned use_lfn;

static char *__get_current_directory(char *out, int drive_number);

static char *
__get_current_directory(char *out, int drive_number)
{
  __dpmi_regs r = {0};
  char tmpbuf[FILENAME_MAX];

  memset(&r, 0, sizeof(r));
  r.x.flags = 1;		/* Set carry for safety */
  if (use_lfn)
    r.x.ax = 0x7147;
  else
    r.h.ah = 0x47;
do_get_current_directory:
  r.h.dl = drive_number + 1;
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
    goto do_get_current_directory;
  }
  else if (r.x.flags & 1)
  {
#ifdef TEST
    errno = __doserr_to_errno(r.x.ax);
    perror("Get dir failed in fixpath");
#endif
    *out++ = '.';	/* Return relative path (lfn=n on Win9x) */
    return out;
  }
  else
  {
    dosmemget(__tb, sizeof(tmpbuf), tmpbuf);
    strcpy(out + 1, tmpbuf);

    if (*(out + 1) != '\0')
    {
      *out = '/';
      return out + strlen(out);
    }
    else if (_os_trueversion != 0x532 || !use_lfn)
      /* Root path, don't insert "/", it'll be added later */
      return out;
  }

  /* Root path under WinNT/2K/XP with lfn (may be silent failure).
     If the short name equivalent of the current path is greater than
     64 characters, Windows 2000 and XP do not return the correct long
     path name - they return the root directory instead without any
     failure code.  Since this can be disastrous in deep directories
     doing an rm -rf, we check for this bug and try and fix the path. */

  r.x.ax = 0x7160;
  r.x.cx = 0x8002;	/* Get Long Path Name, using subst drive basis */
  r.x.es = __tb_segment;
  r.x.di = __tb_offset + FILENAME_MAX;

  tmpbuf[0] = drive_number + 'A';
  tmpbuf[1] = ':';
  tmpbuf[2] = '.';
  tmpbuf[3] = 0;
  _put_path(tmpbuf);

  __dpmi_int(0x21, &r);

  if (!(r.x.flags & 1))
  {
    dosmemget(__tb + FILENAME_MAX, sizeof(tmpbuf), tmpbuf);

    /* Validate return form and drive matches what _fixpath expects. */
    if (IS_DRIVE_SPECIFIER(tmpbuf))
    {
      if (IS_ROOT_DIR(tmpbuf))
        /* Root path, don't insert "/", it'll be added later */
        return out;
      else
      {
        strcpy(out, tmpbuf + 2);  /* Trim drive, just directory */
        return out + strlen(out);
      }
    }
  }
#ifdef TEST
  else
  {
    errno = __doserr_to_errno(r.x.ax);
    perror("Truename failed in fixpath");
  }
#endif

  /* Fixpath failed or returned inconsistent info.  Return relative path. */
  *out++ = '.';
  return out;
}

__inline__ static int
is_slash(int c)
{
  return c == '/' || c == '\\';
}

__inline__ static int
is_term(int c)
{
  return c == '/' || c == '\\' || c == '\0';
}

/* Wrapper for the functions realpath and _fixpath.
   Takes as input an arbitrary path.  Fixes up the path by:
   1. Removing consecutive slashes
   2. Removing trailing slashes
   3. Making the path absolute if it wasn't already
   4. Removing "." in the path
   5. Removing ".." entries in the path (and the directory above them)
   6. Adding a drive specification if one wasn't there
   7. Converting all slashes to '/'
*/

char *
__canonicalize_path(const char *in, char *out, size_t path_max)
{
  int		drive_number;
  char		in1[FILENAME_MAX];
  char		*ip;
  char		*op = out;
  int		preserve_case = _preserve_fncase();
  char		*name_start;
  int		mbsize;
  char		*op_limit;
  int		previous_errno;

  previous_errno = errno;
  use_lfn = _use_lfn(in);
  errno = previous_errno;  /*  Do not signal that LFN API is not available (ENOSYS).  */

  if (path_max > FILENAME_MAX)
    path_max = FILENAME_MAX;

  op_limit = op + path_max;

  /* Perform the same magic conversions that _put_path does.  */
  _put_path(in);
  dosmemget(__tb, sizeof(in1), in1);
  ip = in1;

  /* Add drive specification to output string */
  if (((*ip >= 'a' && *ip <= 'z') ||
       (*ip >= 'A' && *ip <= 'Z'))
      && (*(ip + 1) == ':'))
  {
    if (*ip >= 'a' && *ip <= 'z')
    {
      drive_number = *ip - 'a';
      *op++ = *ip++;
    }
    else
    {
      drive_number = *ip - 'A';
      if (*ip <= 'Z')
	*op++ = drive_number + 'a';
      else
	*op++ = *ip;
      ++ip;
    }
    *op++ = *ip++;
  }
  else
  {
    __dpmi_regs r = {0};
    r.h.ah = 0x19;
    __dpmi_int(0x21, &r);
    drive_number = r.h.al;
    *op++ = drive_number + (drive_number < 26 ? 'a' : 'A');
    *op++ = ':';
  }

  /* Convert relative path to absolute */
  if (!is_slash(*ip))
    op = __get_current_directory(op, drive_number);

  /* Step through the input path */
  while (*ip)
  {
    /* Skip input slashes */
    if (is_slash(*ip))
    {
      ip++;
      continue;
    }

    /* Skip "." and output nothing */
    if (*ip == '.' && is_term(*(ip + 1)))
    {
      ip++;
      continue;
    }

    /* Skip ".." and remove previous output directory */
    if (*ip == '.' && *(ip + 1) == '.' && is_term(*(ip + 2)))
    {
      ip += 2;
      if(out[2] == '.' && *(op - 1) == '.')
      { 				/* relative path not skipped */
        *op++ = '/';
        *op++ = '.';
        *op++ = '.';
      } else
      /* Don't back up over drive spec */
      if (op > out + 2)
	/* This requires "/" to follow drive spec */
	while (!is_slash(*--op));
      continue;
    }

    /* Buffer overflow check.  */
    if (op >= op_limit)
    {
      errno = ENAMETOOLONG;
      return NULL;
    }

    /* Copy path component from in to out */
    *op++ = '/';
#if 0
    while (!is_term(*ip)) *op++ = *ip++;
#else
    while (!is_term(*ip))
      {
	mbsize = mblen (ip, MB_CUR_MAX);
	if (mbsize > 1)
	  {
	    /* copy multibyte character */
	    while (--mbsize >= 0)
	      *op++ = *ip++;
	  }
	else
	  *op++ = *ip++;

        /* Check for buffer overflow.  */
        if (op >= op_limit)
        {
          errno = ENAMETOOLONG;
          return NULL;
        }
      }
#endif
  }

  /* If root directory, insert trailing slash */
  if (op == out + 2) *op++ = '/';

  /* Check for buffer overflow.  */
  if (op >= op_limit)
  {
    errno = ENAMETOOLONG;
    return NULL;
  }

  /* Null terminate the output */
  *op = '\0';

  /* switch FOO\BAR to foo/bar, downcase where appropriate */
  for (op = out + 3, name_start = op - 1; *name_start; op++)
  {
    char long_name[path_max];

#if 1
    /* skip multibyte character */
    mbsize = mblen (op, MB_CUR_MAX);
    if (mbsize > 1)
      {
	op += mbsize - 1;
	continue;
      }
#endif
    if (*op == '\\')
      *op = '/';
    if (!preserve_case && (*op == '/' || *op == '\0'))
    {
      memcpy(long_name, name_start + 1, op - name_start - 1);
      long_name[op - name_start - 1] = '\0';
      if (_is_DOS83(long_name))
      {
#if 0
	while (++name_start < op)
	  if (*name_start >= 'A' && *name_start <= 'Z')
	    *name_start += 'a' - 'A';
#else
	while (++name_start < op)
	  {
	    mbsize = mblen (name_start, MB_CUR_MAX);
	    if (mbsize > 1)
	      {
		/* skip multibyte character */
		name_start += mbsize - 1;
		continue;
	      }
	    else if (*name_start >= 'A' && *name_start <= 'Z')
	      *name_start += 'a' - 'A';
	  }
#endif
      }
      else
	name_start = op;
    }
    else if (*op == '\0')
      break;
  }

  return out;
}

void
_fixpath(const char *in, char *out)
{
  out[0] = '\0';
  if (!in || !in[0])
    return;
  __canonicalize_path(in, out, FILENAME_MAX);
}

#ifdef TEST

int main (int argc, char *argv[])
{
  char fixed[FILENAME_MAX];
  __dpmi_regs r = {0};

  if (argc > 2)
  {
    _put_path(argv[1]);
    r.x.flags = 1;		/* Set carry for safety */
    if (_USE_LFN)
      r.x.ax = 0x713b;
    else
      r.h.ah = 0x3b;
    r.x.dx = __tb_offset;
    r.x.ds = __tb_segment;
    __dpmi_int(0x21, &r);
    if (r.x.ax == 0x7100)
    {
      /*  Never assume that the complete LFN API is implemented,
          so check that AX != 0x7100.  E.G.: MSDOS 6.22 and DOSLFN 0.40. */
      errno = __doserr_to_errno(r.x.ax);
      sprintf(fixed, "Change dir to %s failed (lfn=%d).  LFN driver does not support 0x713B", argv[1], _USE_LFN);
      perror(fixed);
    }
    else if (r.x.flags & 1)
    {
      errno = __doserr_to_errno(r.x.ax);
      sprintf(fixed, "Change dir to %s failed (lfn=%d)", argv[1], _USE_LFN);
      perror(fixed);
    }
    else
      printf("Set dir: %s\n", argv[1]);
    argc--;
    argv++;
  }

  r.x.flags = 1;		/* Set carry for safety */
  if(_USE_LFN)
    r.x.ax = 0x7147;
  else
    r.h.ah = 0x47;
  r.h.dl = 0;
  r.x.si = __tb_offset;
  r.x.ds = __tb_segment;
  __dpmi_int(0x21, &r);
  if (r.x.ax == 0x7100)
  {
    /*  Never assume that the complete LFN API is implemented,
        so check that AX != 0x7100.  E.G.: MSDOS 6.22 and DOSLFN 0.40. */
    errno = __doserr_to_errno(r.x.ax);
    printf("getcwd failed.  LFN driver does not support 0x7147");
    perror("getcwd failed");
  }
  else if (r.x.flags & 1)
  {
    errno = __doserr_to_errno(r.x.ax);
    perror("getcwd failed");
  }
  else
  {
    dosmemget(__tb, sizeof(fixed), fixed);
    printf("Get dir[%d]: \\%s\n", strlen(fixed), fixed);
  }

  if (argc > 1)
  {
    _fixpath (argv[1], fixed);
    printf ("Fixpath: %s\n", fixed);
  }
  return 0;
}

#endif
