/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <unistd.h>
#include <libc/stubs.h>
#include <dpmi.h>

extern int __brk(void *);

int
brk(void *_heaptop)
{
  if (__brk(_heaptop) == -1)
    return -1;
  return 0;
}

void *sbrk(int delta)
{
  return DATA_PTR(__sbrk(delta));
}
