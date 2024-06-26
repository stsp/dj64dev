/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2004 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <libc/unconst.h>
#include <libc/environ.h>
#include <process.h>

extern char **_environ;

int spawnl(int mode, const char *path, const char *argv0, ...)
{
  return spawnve(mode, path, unconst(&argv0,char * const *), _environ);
}
