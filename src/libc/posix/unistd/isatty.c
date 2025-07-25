/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <unistd.h>
#include <io.h>
#include <libc/getdinfo.h>

#define ISATTY_BITS (_DEV_CDEV|_DEV_STDIN|_DEV_STDOUT)

int
isatty(int fd)
{
  const int dev_info = _get_dev_info(fd);

  /* Pass on errno from _get_dev_info. */
  if (dev_info == -1)
    return 0;

  if ((dev_info & ISATTY_BITS) == ISATTY_BITS)
    return 1;
  return 0;
}

#ifdef __APPLE__
int __isatty(int fd);
int __isatty(int fd)
{
  return isatty(fd);
}
#else
int __attribute__((alias("isatty"))) __isatty(int fd);
#endif
