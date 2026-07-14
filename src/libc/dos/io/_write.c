/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <go32.h>
#include <dpmi.h>
#include <io.h>
#include <sys/fsext.h>
#include <libc/fsexthlp.h>
#include <libc/dosio.h>
#include <libc/fd_props.h>
#include <libc/farptrgs.h>
#include <libc/getdinfo.h>

int
_write(int handle, const void* buffer, unsigned count)
{
  __FSEXT_Function *func = __FSEXT_get_function(handle);
  if (func)
  {
    int rv;
    if (__FSEXT_func_wrapper(func, __FSEXT_write, &rv, handle, buffer, count))
      return rv;
  }

  if (__has_fd_properties(handle)
      && (__fd_properties[handle]->flags & FILE_DESC_ZERO_FILL_EOF_GAP))
  {
    if (_write_fill_seek_gap(handle) < 0)
      return -1;
  }

  return _write_int(handle, (const char *)buffer, count);
}

/* If the file pointer offset is beyond EOF, fill the gap between EOF and
   the file pointer offset with zeroes.  This emulates the behavior described
   in the POSIX documentation for lseek.  */
int
_write_fill_seek_gap(int fd)
{
  offset_t eof_off, cur_off;
  size_t fill_count;
  unsigned long buf_size;
  unsigned long i;
  short fd_info;

  __clear_fd_flags(fd, FILE_DESC_ZERO_FILL_EOF_GAP);

  /* Quit when there can't be an EOF gap or its existance doesn't matter.  */
  if (__fd_properties[fd]->flags & FILE_DESC_DONT_FILL_EOF_GAP)
    return 0;

  /* Quit when not working with a file.  */
  fd_info = _get_dev_info(fd);
  if (fd_info & _DEV_CDEV)
  {
    /* Don't bother with handles that don't need the fix.  */
    __set_fd_flags(fd, FILE_DESC_DONT_FILL_EOF_GAP);
    return 0;
  }

  /* Quit when unable to get the file length.  */
  eof_off = lfilelength (fd);
  if (eof_off < 0)
    return 0;

  /* Quit when unable to get the current file offset.  */
  cur_off = llseek (fd, 0, SEEK_CUR);
  if (cur_off < 0)
    return 0;

  /* Quit if the current offset is not past EOF.  */
  if (cur_off <= eof_off)
    return 0;

  /* Quit when unable to seek to EOF.  */
  if (llseek (fd, eof_off, SEEK_SET) == -1)
    return 0;

  /* Clear once again because the llseek call above will
     set the fill test flag.  */
  __clear_fd_flags(fd, FILE_DESC_ZERO_FILL_EOF_GAP);

  /* Fill the transfer buffer with zeros.  */
  fill_count = cur_off - eof_off;

  buf_size = (fill_count > __tb_size) ? __tb_size : fill_count;

  i = 0;
  _farsetsel(_dos_ds);
  while (i < buf_size)
  {
    _farnspokel(__tb + i, 0);
    i += 4;
  }

  /* Write out 'fill_count' number of zeros.  */
  /* Warning! If fill_count > ULONG_MAX, this call won't work.
     But changing _write_int's last argument to 'unsigned long long'
     won't work either because gcc generates bad code for long longs
     passed via the stack.  */
  return _write_int(fd, NULL, fill_count);
}

/* Write WRITE_COUNT bytes of data to the file associated with FD.
   If BUFFER is not NULL, the data pointed to by BUFFER is put into the
   transfer buffer and written out. Otherwise, the data already in the
   transfer buffer is written out. */
int
_write_int(int fd, const char *buffer, unsigned long long write_count)
{
  unsigned long buf_size;
  unsigned long chunk_count;
  int total_written;
  unsigned short bytes_written;
  __dpmi_regs r = {0};

  buf_size = (write_count <= __tb_size) ? write_count : __tb_size;

  total_written = 0;
  do
  {
    chunk_count = (write_count <= buf_size) ? write_count : buf_size;
    if (buffer && chunk_count)
      dosmemput(buffer, chunk_count, __tb);
    r.x.ax = 0x4000;
    r.x.bx = fd;
    r.x.cx = chunk_count;
    r.x.dx = __tb & 15;
    r.x.ds = __tb / 16;
    __dpmi_int(0x21, &r);
    if (r.x.flags & 1)
    {
      errno =__doserr_to_errno(r.x.ax);
      return -1;
    }
    bytes_written = r.x.ax;
    write_count -= bytes_written;
    total_written += bytes_written;
    if (buffer)
      buffer += bytes_written;
  } while (write_count && (chunk_count == bytes_written));

  if (write_count && total_written == 0)
  {
    errno = ENOSPC;
    return -1;
  }

  return total_written;
}
