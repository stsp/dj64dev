/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2004 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <libc/environ.h>
#include <process.h>

extern char **_environ;

int spawnv(int mode, const char *path, char *const argv[])
{
  return spawnve(mode, path, (char * const *)argv, _environ);
}
