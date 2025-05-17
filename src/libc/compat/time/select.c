/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/* An implementation of select()

   Copyright 1995 by Morten Welinder
   This file maybe freely distributed and modified as long as the
   copyright notice remains.

   Notes: In a single process system as Dos this really boils down to
   something that can check whether a character from standard input
   is ready.  However, the code is organised in a way to make it easy
   to extend to multi process systems like WinNT and OS/2.  */

#include <libc/stubs.h>
#include <sys/types.h>
#include <sys/select.h>
#include <dpmi.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <libc/file.h>
#include <libc/local.h>
#include <libc/dosio.h>
#include <libc/getdinfo.h>
#include <libc/ttyprvt.h>
#include <sys/fsext.h>
#include <libc/fsexthlp.h>
#include <io.h>
#include <libc/fd_props.h>

inline static int
fp_output_ready(FILE *fp)
{
  return !ferror(fp);
}

/* This is as close as we get, I think.  For a file connected to a printer
   we could of course go ask the BIOS, but this should be enough.  */

inline static int
fp_except_ready(FILE *fp)
{
  return ferror (fp);
}

inline static int
fp_input_ready (FILE *fp)
{
  /* I think if there is something in the buffer, we should return
     ``ready'', even if some error was encountered.  Let him consume
     the buffered characters, *then* return ``not ready''.  */
  if (fp->_cnt)
    return 1;

  if (ferror (fp))
    return 0;

  /* There is nothing in the buffer (perhaps because we read unbuffered).
     We don't know if we are ready.  Return ``ready'' anyway and let
     read() or write() tell the truth.  */
  return 1;
}

/* The Dos call 4407 always returns TRUE for disk files.  So the
   following really is meaningful for character devices only...  */

inline static int
fd_output_ready(int fd)
{
  __dpmi_regs regs = {0};

  /* If it's a directory, always return 0. We can't write to directories. */
  if (__get_fd_flags(fd) & FILE_DESC_DIRECTORY)
    return 0;

  regs.x.ax = 0x4407;
  regs.x.bx = fd;
  __dpmi_int (0x21, &regs);
  if (regs.x.flags & 1)
  {
    errno = __doserr_to_errno (regs.x.ax);
    return -1;
  }
  else
    return regs.h.al == 0xff;
}

inline static int
fd_input_ready(int fd)
{
  __dpmi_regs regs = {0};
  short dev_info;

  /* If it's a directory, always return 1. That way the caller
     will hit the EISDIR error as quickly as possible. */
  if (__get_fd_flags(fd) & FILE_DESC_DIRECTORY)
    return 1;

  /* If it's a disk file, always return 1, since DOS returns ``not ready''
     for files at EOF, but we won't block in that case.  */
  dev_info = _get_dev_info(fd);
  if (dev_info == -1)
    return -1;

  if ((dev_info & _DEV_CDEV) == 0)	/* if not a character device */
    return 1;

  /* If it's a STDIN device, and termios buffers some characters, say
     it's ready for input.  */
  else if ((dev_info & _DEV_STDIN) == _DEV_STDIN
	   && __libc_read_termios_hook && __libc_termios_exist_queue ())
    return 1;

  regs.x.ax = 0x4406;
  regs.x.bx = fd;
  __dpmi_int (0x21, &regs);
  if (regs.x.flags & 1)
  {
    errno = __doserr_to_errno (regs.x.ax);
    return -1;
  }
  else
    return regs.h.al == 0xff;
}

int
select(int nfds,
	fd_set *readfds,
	fd_set *writefds,
	fd_set *exceptfds,
	struct timeval *timeout)
{
  int ready;
  fd_set oread, owrite, oexcept;
  struct timeval now, then;

  if (nfds > FD_SETSIZE)
  {
    errno = EINVAL;
    return -1;
  }

  FD_ZERO (&oread);
  FD_ZERO (&owrite);
  FD_ZERO (&oexcept);
  ready = 0;

  then.tv_sec = 0;
  then.tv_usec = 0;
  if (timeout)
  {
    if (timeout->tv_usec < 0)
    {
      errno = EINVAL;
      return -1;
    }
    gettimeofday (&now, 0);
    then.tv_usec = timeout->tv_usec + now.tv_usec;
    then.tv_sec = timeout->tv_sec + now.tv_sec + then.tv_usec / 1000000;
    then.tv_usec %= 1000000;
  }

  do {
    int i;
    int fd0 = 0;
    __file_rec *fr = __file_rec_list;
    FILE *fp;

    /* First, check the file handles with low-level DOS calls.  */
    for (i = 0; i < nfds; i++)
    {
      int ioctl_result;
      __FSEXT_Function *func = __FSEXT_get_function(i);
      int fsext_ready = -1;

      if (func)
	__FSEXT_func_wrapper(func, __FSEXT_ready, &fsext_ready, i);

      if (readfds && FD_ISSET (i, readfds))
      {
	if (fsext_ready != -1)
	{
	  if (fsext_ready & __FSEXT_ready_read)
	    ready++, FD_SET(i, &oread);
	}
        else if ((ioctl_result = fd_input_ready (i)) == -1)
          return -1;
        else if (ioctl_result)
          ready++, FD_SET (i, &oread);
      }
      if (writefds && FD_ISSET (i, writefds))
      {
        if (fsext_ready != -1)
	{
	  if (fsext_ready & __FSEXT_ready_write)
	    ready++, FD_SET(i, &owrite);
	}
        else if ((ioctl_result = fd_output_ready (i)) == -1)
          return -1;
        else if (ioctl_result)
          ready++, FD_SET (i, &owrite);
      }
      if (exceptfds && FD_ISSET (i, exceptfds))
      {
        if (fsext_ready != -1)
	{
	  if (fsext_ready & __FSEXT_ready_error)
	    ready++, FD_SET(i, &oexcept);
	}
      }
    }

    /* Now look at the table of FILE ptrs and reset the bits for file
       descriptors which we *thought* are ready, but for which the flags
       say they're NOT ready.  */
    for (i = 0; fr; i++)
    {
      if (i >= fd0 + fr->count) /* done with this subtable, go to next */
      {
	fd0 += fr->count;
	fr = fr->next;
      }
      if (fr)
      {
        fp = fr->files[i - fd0];
        if (fp->_flag)
        {
          int this_fd = fileno(fp);

          if (this_fd < nfds)
          {
            if (readfds && FD_ISSET (this_fd, readfds) &&
                FD_ISSET (this_fd, &oread) && !fp_input_ready (fp))
              ready--, FD_CLR (this_fd, &oread);
            if (writefds && FD_ISSET (this_fd, writefds) &&
                FD_ISSET (this_fd, &owrite) && !fp_output_ready (fp))
              ready--, FD_CLR (this_fd, &owrite);

            /* For exceptional conditions, ferror() is the only one
               which can tell us an exception is pending.  */
            if (exceptfds && FD_ISSET (this_fd, exceptfds) &&
                fp_except_ready (fp))
              ready++, FD_SET (this_fd, &oexcept);
          }
        }
      }
    }

    /* Exit if we found what we were waiting for.  */
    if (ready > 0)
    {
      if (readfds)
	*readfds = oread;
      if (writefds)
	*writefds = owrite;
      if (exceptfds)
	*exceptfds = oexcept;
      return ready;
    }

    /* Exit if we hit the time limit.  */
    if (timeout)
    {
      gettimeofday (&now, 0);
      if (now.tv_sec > then.tv_sec
	  || (now.tv_sec == then.tv_sec && now.tv_usec >= then.tv_usec))
      {
	if (readfds)
	  FD_ZERO (readfds);
	if (writefds)
	  FD_ZERO (writefds);
	if (exceptfds)
	  FD_ZERO (exceptfds);
	return 0;
      }
    }

    /* We are busy-waiting, so give other processes a chance to run.  */
    __dpmi_yield ();
  } while (1);
}

#ifdef  TEST

#include <stdio.h>
#include <stdlib.h>

int
main(void)
{
  struct timeval timeout;
  fd_set read_fds, write_fds;
  int i, select_result;

  timeout.tv_sec = 15;
  timeout.tv_usec = 0;

  /* Display status of the 5 files open by default.  */
  for (i = 0; i < 5; i++)
    {

      FD_ZERO (&read_fds);
      FD_SET (i, &read_fds);
      select_result = select (i + 1, &read_fds, 0, 0, &timeout);
      if (select_result == -1)
        {
          fprintf(stderr, "%d: Failure for input", i);
          perror("");
        }
      else
        fprintf(stderr,
                "%d: %s ready for input\n", i, select_result ? "" : "NOT");
      FD_ZERO (&write_fds);
      FD_SET (i, &write_fds);
      select_result = select (i + 1, 0, &write_fds, 0, &timeout);
      if (select_result == -1)
        {
          fprintf(stderr, "%d: Failure for output", i);
          perror("");
        }
      else
        fprintf(stderr,
                "%d: %s ready for output\n", i, select_result ? "" : "NOT");
    }
  return 0;
}

#endif
