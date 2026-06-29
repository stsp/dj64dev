/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/internal.h>
#include <libc/bss.h>

void
__main(void)
{
#if 0
  static int been_there_done_that = -1;
  int i;
  if (been_there_done_that == __bss_count)
    return;
  been_there_done_that = __bss_count;
  for (i=0; i<djgpp_last_ctor-djgpp_first_ctor; i++)
    _djgpp_first_ctor(i);
#endif
}
