/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/internal.h>
#include <libc/bss.h>

#if !USE64
typedef void (*FUNC)(void);
extern FUNC __init_array_start[] __attribute__((weak));
extern FUNC __init_array_end[] __attribute__((weak));
#endif

void
__main(void)
{
#if !USE64
  static int been_there_done_that = -1;
  int i;
  int count = __init_array_end - __init_array_start;
  if (been_there_done_that == __bss_count)
    return;
  been_there_done_that = __bss_count;
  for (i = 0; i < count; i++)
    __init_array_start[i]();
#endif
  djprocess_init_hooks();
}
