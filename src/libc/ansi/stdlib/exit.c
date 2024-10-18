/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <libc/internal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dos.h>
#include <io.h>
#include <libc/atexit.h>
#include <libc/stdiohk.h>
#include <libc/djctx.h>

struct __atexit *__atexit_ptr = 0;

extern void (*__stdio_cleanup_hook)(void); /* stdio/stdiohk.c */

/* A hook to close down the file system extensions if any where opened.
   This does not force them to be linked in. */
void (*__FSEXT_exit_hook)(void) = NULL;

/* A hook to close those file descriptors with properties. */
void (*__fd_properties_cleanup_hook)(void) = NULL;

struct exit_state {
  struct __atexit *__atexit_ptr;
  void (*__FSEXT_exit_hook)(void);
  void (*__fd_properties_cleanup_hook)(void);
  void (*atexit2_hook)(void *, int);
  void *atexit2_arg;
};

static struct exit_state *est;
static void est_pre(void)
{
#define _SV(x) est->x = x
  _SV(__atexit_ptr);
  _SV(__FSEXT_exit_hook);
  _SV(__fd_properties_cleanup_hook);
}
static void est_post(void)
{
#define _RS(x) x = est->x
  _RS(__atexit_ptr);
  _RS(__FSEXT_exit_hook);
  _RS(__fd_properties_cleanup_hook);
}
DJ64_DEFINE_SWAPPABLE_CONTEXT2(exit_state, est, ((struct exit_state){}),
    est_pre(), est_post());

#define atexit2_hook est->atexit2_hook
#define atexit2_arg est->atexit2_arg

void
exit(int status)
{
//  int i;
  struct __atexit *a,*o;

  a = __atexit_ptr;
  __atexit_ptr = 0; /* to prevent infinite loops */
  while (a)
  {
    (a->__function)();
    o = a;
    a = a->__next;
    free(o);
  }
#if 0
  /* Destructors should probably be called after functions registered
     with atexit(), this is the way it happens in Linux anyway. */
  for (i=0; i<djgpp_last_dtor-djgpp_first_dtor; i++)
    _djgpp_first_dtor(i);
#endif
  /* Do this last so that everyone else may write to files
     during shutdown */
  if (__stdio_cleanup_hook)
    __stdio_cleanup_hook();

  if (__fd_properties_cleanup_hook)
    __fd_properties_cleanup_hook();

  /* Do this after the stdio cleanup to let it close off the fopen'd files */
  if (__FSEXT_exit_hook)
    __FSEXT_exit_hook();

  /* in case the program set it this way */
  setmode(0, O_TEXT);

  if (atexit2_hook)
    atexit2_hook(atexit2_arg, status);
  _exit(status);
}

int atexit2(void (*function)(void *, int), void *arg)
{
  if (atexit2_hook && function)
    return -1;
  atexit2_hook = function;
  atexit2_arg = arg;
  return 0;
}
