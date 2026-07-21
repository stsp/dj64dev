/*
 *  dj64 - 64bit djgpp-compatible tool-chain
 *  Copyright (C) 2026  @stsp
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <libc/file.h>
#include <libc/fd_props.h>

FILE *
fdreopen(int fd, const char *mode, FILE *f)
{
  int fdo, rw;

  if (fd == -1 || mode == NULL || f == NULL)
    return NULL;

  rw = (mode[1] == '+') || (mode[1] && (mode[2] == '+'));

  fdo = fileno(f);
  if (fdo == fd)
    return f;
  fclose(f);

  if (fdo >= 0)
  {
    dup2(fd, fdo);  /* Might rarely happen, but if it does for */
    fd = fdo;       /* child processes won't popen properly.  */
  }

  f->_cnt = 0;
  f->_file = fd;
  f->_bufsiz = 0;
  if (rw)
    f->_flag = _IORW;
  else if (*mode == 'r')
    f->_flag = _IOREAD;
  else
    f->_flag = _IOWRT;

  if (*mode == 'a')
    llseek(fd, 0LL, SEEK_END);

  f->_base = f->_ptr = NULL;

  if (__get_fd_flags(fd) & FILE_DESC_DIRECTORY)
  {
    f->_flag &= ~(_IORW|_IOREAD|_IOWRT);
    f->_flag |= _IOEOF;
  }

  return f;
}
