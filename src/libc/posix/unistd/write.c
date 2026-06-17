/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <go32.h>
#include <dpmi.h>
#include <io.h>
#include <errno.h>
#include <libc/farptrgs.h>
#include <sys/fsext.h>
#include <libc/fsexthlp.h>
#include <libc/dosio.h>
#include <libc/ttyprvt.h>
#include <libc/fd_props.h>

ssize_t
write(int handle, const void* buffer, size_t count)
{
  const char *buf = (const char *)buffer;
  size_t bytes_in_tb = 0;
  size_t offset_into_buf = 0;
  int out;
  ssize_t ss_rv;
  int i_rv;
  __FSEXT_Function *func = __FSEXT_get_function(handle);

  /* termios special hook */
  if (__libc_write_termios_hook != NULL)
      if (__libc_write_termios_hook(handle, buffer, count, &ss_rv) != 0)
        return ss_rv;

  if (count == 0)
    return 0; /* POSIX.1 requires this */

  /* Directory? If so, fail. */
  if (__get_fd_flags(handle) & FILE_DESC_DIRECTORY)
  {
    errno = EBADF;
    return -1;
  }

  if ( __has_fd_properties(handle)
  && ( __fd_properties[handle]->flags & FILE_DESC_APPEND ) )
  {
    if ( llseek(handle, 0, SEEK_END) == -1 )
    {
      /* llseek() sets errno. */
      return -1;
    }
  }

  if(__file_handle_modes[handle] & O_BINARY)
    return _write(handle, buf, count);

  /* Let's handle FSEXT_write ! */
  /* if handler is installed, call extension and exit if handled. */
  if(func &&
     __FSEXT_func_wrapper(func, __FSEXT_write, &i_rv, handle, buffer, count))
      return i_rv;

  if (__has_fd_properties(handle)
      && (__fd_properties[handle]->flags & FILE_DESC_ZERO_FILL_EOF_GAP))
  {
    if (_write_fill_seek_gap(handle) < 0)
      return -1;
  }

  while (offset_into_buf < count)
  {
    _farsetsel(_dos_ds);
    while (bytes_in_tb < __tb_size && offset_into_buf < count)
    {
      if (buf[offset_into_buf] == '\n')
      {
	if (bytes_in_tb == __tb_size - 1)
	  break; /* can't fit two more */
	_farnspokeb(__tb + bytes_in_tb, '\r');
	bytes_in_tb++;
      }
      _farnspokeb(__tb + bytes_in_tb, buf[offset_into_buf]);
      bytes_in_tb++;
      offset_into_buf++;
    }

    /* Write out the contents of the transfer buffer. */
    out = _write_int(handle, NULL, bytes_in_tb);
    if (out < 0)
      return -1;
    if ((size_t)out < bytes_in_tb)
    {
      offset_into_buf += out;
      errno = ENOSPC;
      return count - offset_into_buf;
    }

    bytes_in_tb = 0;
  }

  return count;	/* Don't return count with CR's (TC) */
}
