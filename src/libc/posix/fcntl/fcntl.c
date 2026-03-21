/* Copyright (C) 2019 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2016 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2010 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <dpmi.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <io.h>
#include <sys/fsext.h>
#include <libc/fsexthlp.h>
#include <sys/movedata.h>
#include <libc/farptrgs.h>
#include <libc/dosio.h>
#include <libc/getdinfo.h>
#include <libc/fd_props.h>
#include <go32.h>


static unsigned long _get_sft_entry_ptr(int fd)
{
  __dpmi_regs regs = {0};
  unsigned char ind;
  unsigned long es, di;


  /* Get the JFT entry address for this handle.  */
  regs.x.ax = 0x1220;
  regs.x.bx = fd;
  __dpmi_int(0x2f, &regs);


  if (regs.x.flags & 1) /* int2F/1220 returns CF set if bad input */
  {
    errno = __doserr_to_errno(regs.h.al);  /* And AL has the error number */
    return 0;
  }


  /* Get the SFT entry number for this handle.  */
  es = regs.x.es;
  di = regs.x.di;
  ind = _farpeekb(_dos_ds, es * 16 + di);


  if (ind < 0xff)
  {
    /* Now get the address of the entry.  */
    regs.x.ax = 0x1216;
    regs.x.bx = ind;
    __dpmi_int (0x2f, &regs);
  }
  else
    regs.x.flags |= 1;  /* Set carry flag to simulate error       */

  if (regs.x.flags & 1) /* int2F/1216 returns CF set if bad input */
  {
    errno = EBADF;      /* But no other information is returned   */
    return 0;
  }


  es = regs.x.es;
  di = regs.x.di;


  return es * 16 + di;
}


static int
inherit_bit_test (int fd, short dev_info)
{
  __dpmi_regs regs = {0};
  short new_dev_info;


  dev_info |= _DEV_NO_INHERIT;
  regs.x.ax = 0x4401;
  regs.x.bx = fd;
  regs.x.dx = dev_info;
  __dpmi_int(0x21, &regs);

  new_dev_info = _get_dev_info(fd);

  /* If the dev info words are equal, then the documented
     interface can be used to set the inheritance bit.  */
  return (new_dev_info == dev_info) ? 1 : 0;
}

static int
_get_SHARE_status (void)
{
  __dpmi_regs regs = {0};

  regs.x.ax = 0x1000;
  __dpmi_int(0x2f, &regs);

  /* If al == 0xff then SHARE is installed, otherwise not */
  return (regs.h.al == 0xff) ? 1 : 0;
}

static int
_fcntl_lk64(int fd, int cmd, struct flock64 *lock_r64)
{
  int ret = -1;
  long long int ret64 = -1L;
  offset_t pos, cur_pos, lock_pos, len;
  long long int flen;

  /* Is this a directory? If so, fail. */
  if (__get_fd_flags(fd) & FILE_DESC_DIRECTORY)
  {
    errno = EINVAL;
    return -1;
  }

  /* Check if SHARE is loaded */
  ret = _get_SHARE_status();

  if (!ret) /* Then SHARE is NOT loaded, just return success */
  {
    if (cmd == F_GETLK64) /* Then make believe the lock is available */
      lock_r64->l_type = F_UNLCK;

    return ret;
  }
  else ret = -1; /* Restore default value */

  cur_pos = llseek(fd, 0L, SEEK_CUR);
  if (cur_pos < 0)
    return -1;      /* Assumes llseek has set errno */

  lock_pos = llseek (fd, lock_r64->l_start, lock_r64->l_whence);
  if (lock_pos < 0)
    return -1;      /* Assumes llseek has set errno */

  len = lock_r64->l_len;


  /* If l_len is zero, then the lock is to be set from the position
     represented by l_start/l_whence to the current end-of-file.
  */
  if (len == 0L)
  {
    flen = lfilelength(fd); /* Hold lfilelength in case len < 0 */
    len = flen - lock_pos;  /* len > 0      lock_pos before EOF
                               len == 0     lock_pos at     EOF
                               len < 0      lock_pos after  EOF
                            */

    /* If now len < 0, then lock_pos is beyond EOF, and
       the code below will calculate the correct region.
    */
    if (len < 0L)
    {
      pos = flen;   /* Start at EOF == lfilelength(fd) */
      lock_pos = flen; /* Also set lock_pos for later  */
      len = -len;   /* Make the length positive        */
    }
  }


  /* Return to saved position */
  ret64 = llseek (fd, cur_pos, SEEK_SET);
  if (ret64 < 0L)
    return -1;      /* Assumes llseek has set errno */
  else
    ret64 = -1;     /* Restore default value */


  /* If l_len is positive, the area to lock is from l_start
     to l_start + l_len - 1. If l_len is negative, the area to lock is
     from l_start + len to l_start - 1.

     Note that if l_len == 0, then pos and len may have already
     been set above, and lock_pos may have been reset to flen.
  */
  if (len > 0L)
  {
    pos = lock_pos;
  }
  else
  {
    pos = lock_pos + len;
    len = -len;
  }


  /* DOS/Win9x only support write locks via int 21/5C, so
     all read lock requests are treated like write locks
  */

  if (lock_r64->l_type == F_UNLCK)
  {
    ret = _dos_unlk64(fd, pos, len);
    if (ret != 0)
    {
      _doserrno = ret;
      errno = __doserr_to_errno(ret);
      return -1;
    }
  }
  else if ((lock_r64->l_type == F_WRLCK) ||
           (lock_r64->l_type == F_RDLCK))
  {
    ret = _dos_lk64(fd, pos, len);
    if (cmd == F_GETLK64)
    {
      if (!ret)
      {
        _dos_unlk64(fd, pos, len);
        /* If no lock is found that would prevent a lock from
           being created, the lock type is set to F_UNLCK.
        */
        lock_r64->l_type = F_UNLCK;
      }
      else
      {
        /* If a lock is found then l_whence, l_start, and l_len
           should point to the area covered by the lock. But the
           file locking interface doesn't report where the
           existing lock is, so nothing more can be done.
        */
        _doserrno = ret;
        errno = __doserr_to_errno(ret);
        return -1;
      }
    }
    else
    {
      /* If F_SETLKW64 is set, wait for the lock to be released.  */
      if (cmd == F_SETLKW64 && ret)
      {
        while ((ret = _dos_lk64(fd, pos, len)))
          __dpmi_yield();
      }
      else if (ret)
      {
        _doserrno = ret;
        errno = __doserr_to_errno(ret);
        return -1;
      }
    }
  }
  if (ret)
    errno = ENOSYS;

  return ret;
}


int
fcntl(int fd, int cmd, ...)
{
  int tofd, open_max;
  va_list ap;
  __FSEXT_Function *func;
  short dev_info = _get_dev_info(fd);
  static int inherit_bit_visible = -1;
  int errno_save;

  /* Verify the descriptor is valid by retrieving
     the handle's device info word.  */
  if (dev_info == -1)
  {
    errno = EBADF;
    return dev_info;
  }


  /* Allow a fd to override with a FSEXT.  */
  func = __FSEXT_get_function(fd);
  if (func)
  {
    int rv, arg;

    va_start(ap, cmd);
    arg = va_arg(ap, int);
    va_end(ap);

    if (__FSEXT_func_wrapper(func, __FSEXT_fcntl, &rv, fd, cmd, arg))
      return rv;
  }


  errno_save = errno;

  switch (cmd)
  {
    case F_DUPFD:
    {
      va_start(ap, cmd);
      tofd = va_arg(ap, int);
      va_end(ap);


      open_max = getdtablesize();
      while (tofd < open_max)
      {
        /* If unable to get the device info for the handle,
           then the handle is not active and it can be used.  */
        if (_get_dev_info(tofd) == -1)
          break;
        tofd++;
      }


      if (tofd >= open_max)
      {
        errno = EINVAL;
        return -1;
      }


      errno = (tofd = dup2(fd, tofd)) == -1 ? EINVAL : errno_save;
      return tofd;
    }


    case F_GETFD:
    {
       unsigned long entry_ptr;


      /* DOS only passes the first 20 handles to child programs.  In
         addition, handles 19 and 18 will be closed by the stub of the
         child program (if it is a DJGPP program).  */
      if (fd >= 18)
        return FD_CLOEXEC;


      /* Determine if the documented interface will allow twiddling with
         the inherit bit. If not, fallback to the undocumented one.  */
      if (inherit_bit_visible == -1)
        inherit_bit_visible = inherit_bit_test (fd, dev_info);

      if (!inherit_bit_visible)
      {
        entry_ptr = _get_sft_entry_ptr(fd);
        if (entry_ptr == 0)
        {
          /* The fd has already been validated, so reaching here means
             something is wrong with _get_sft_entry_ptr. */
          return -1;
        }
        /* Offset 5 in the SFT contains the device info word.  */
        dev_info = _farpeekw(_dos_ds, entry_ptr + 5);
      }
      return (dev_info & _DEV_NO_INHERIT) ? FD_CLOEXEC : 0;
    }


    case F_SETFD:
    {
      unsigned int flag;
      unsigned long entry_ptr = 0; /* shut up -Wall */
      __dpmi_regs regs = {0};


      va_start (ap, cmd);
      flag = va_arg(ap, int);
      va_end(ap);


      /* DOS only passes the first 20 handles to child programs.  In
         addition, handles 19 and 18 will be closed by the stub of the
         child program (if it is a DJGPP program).  */
      if (fd >= 18)
      {
        if (flag & FD_CLOEXEC)
          return 0;
        else
        {
          errno = ENOSYS;
          return -1;
        }
      }


      /* Determine if the documented interface will allow twiddling with
         the inherit bit. If not, fallback to the undocumented one.  */
      if (inherit_bit_visible == -1)
        inherit_bit_visible = inherit_bit_test(fd, dev_info);

      if (!inherit_bit_visible)
      {
        entry_ptr = _get_sft_entry_ptr(fd);
        if (entry_ptr == 0)
        {
          /* The fd has already been validated, so reaching here means
             something is wrong with _get_sft_entry_ptr. */
          return -1;
        }


        dev_info = _farpeekw(_dos_ds, entry_ptr + 5);
      }


      if (flag & FD_CLOEXEC)
        dev_info |= _DEV_NO_INHERIT;
      else
        dev_info &= ~(_DEV_NO_INHERIT);


      if (inherit_bit_visible)
      {
        regs.x.ax = 0x4401;
        regs.x.bx = fd;
        regs.x.dx = dev_info;
        __dpmi_int(0x21, &regs);
      }
      else
        _farpokew(_dos_ds, entry_ptr + 5, dev_info);


      return 0;
    }


    case F_GETFL:
    {
      unsigned long sft_entry_ptr;
      unsigned char sft_open_flags;
      int flags = 0;


      /*
       * Use the data in the SFT.
       *
       * This seems to work on Windows 2000 and therefore probably works
       * on Windows XP. It does not work on Windows NT 4, where the file
       * will always appear to have been opened read-only. It's hard
       * to distinguish Windows 2000 and Windows NT 4 with an LFN TSR,
       * so we can't have a special case for Windows NT 4.
       */
      sft_entry_ptr = _get_sft_entry_ptr(fd);
      if (sft_entry_ptr)
      {
	/* Offset 2 in the SFT contains the open flags. */
	sft_open_flags = _farpeekw(_dos_ds, sft_entry_ptr + 2);

	switch(sft_open_flags & 0x7)
	{
	case 0: flags |= O_RDONLY; break;
	case 1: flags |= O_WRONLY; break;
	case 2: flags |= O_RDWR;   break;

	default:
	  break;
	}
      }
      else
      {
	/* If we've failed to get an SFT pointer, pretend that the file
	 * is read-write, since that's most likely to be correct. */
	flags |= O_RDWR;
      }

      /* Check the FILE_DESC_APPEND flag in __fd_properties. */
      if (__has_fd_properties(fd))
      {
	unsigned long fd_flags = __get_fd_flags(fd);

	if (fd_flags & FILE_DESC_APPEND)
	  flags |= O_APPEND;
      }

      return flags;
    }


    case F_SETFL:
    {
      unsigned char new_mode_bits;


      va_start (ap, cmd);
      new_mode_bits = va_arg(ap,int);
      va_end(ap);


      /* Allow removal of O_NONBLOCK, since DJGPP doesn't support it
         anyway.  */
      return (new_mode_bits == 0) ? 0 : -1;
    }


    case F_GETLK:
    case F_SETLK:
    case F_SETLKW:
    {
      struct flock *lock_req = NULL; /* shut up -Wall */
      struct flock64 lock_r64;
      int ret = -1, lcmd = -1;


      va_start (ap, cmd);
      lock_req = va_arg(ap, struct flock *);
      va_end (ap);

      switch (cmd)
      {
        case F_GETLK:   lcmd = F_GETLK64;  break;
        case F_SETLK:   lcmd = F_SETLK64;  break;
        case F_SETLKW:  lcmd = F_SETLKW64; break;
      }

      lock_r64.l_type   = lock_req->l_type;
      lock_r64.l_start  = lock_req->l_start;
      lock_r64.l_whence = lock_req->l_whence;
      lock_r64.l_len    = lock_req->l_len;

      ret = _fcntl_lk64(fd, lcmd, &lock_r64);

      lock_req->l_type   =         lock_r64.l_type;
      lock_req->l_start  = (off_t) lock_r64.l_start;
      lock_req->l_whence =         lock_r64.l_whence;
      lock_req->l_len    = (off_t) lock_r64.l_len;

      return ret;
    }


    case F_GETLK64:
    case F_SETLK64:
    case F_SETLKW64:
    {
      struct flock64 *lock_r64; /* shut up -Wall */
      int ret;


      va_start (ap, cmd);
      lock_r64 = va_arg(ap, struct flock64 *);
      va_end (ap);

      ret = _fcntl_lk64(fd, cmd, lock_r64);

      return ret;
    }


    case F_GETOWN:
    case F_SETOWN:
    {
      errno = ENOSYS;
      return -1;
    }
 }


  /* In case fcntl is called with an unrecognized command.  */
  errno = EINVAL;
  return -1;
}
