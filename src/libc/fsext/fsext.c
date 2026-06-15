/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <stdlib.h>
#include <go32.h>
#include <dpmi.h>
#include <errno.h>
#include <sys/fsext.h>
#include <libc/bss.h>
#include <libc/dosio.h>
#include <unistd.h>
#include <fcntl.h>
#include <io.h>
#include <libc/djctx.h>

typedef struct __FSEXT_entry {
  __FSEXT_Function *function;
  void *data;
} __FSEXT_entry;

struct fsext_state {
  int num_fds;
  __FSEXT_entry *fsext_list;
};
#if USE64
static struct fsext_state *fst;
DJ64_DEFINE_SWAPPABLE_CONTEXT(fsext_state, fst)
#define num_fds fst->num_fds
#define fsext_list fst->fsext_list
#else
static struct fsext_state fst;
#define num_fds fst.num_fds
#define fsext_list fst.fsext_list
#endif

extern void (*__FSEXT_exit_hook)(void);
static void __FSEXT_close_all(void);

static int
init(void)
{
  static int init_count = -1;
  if (init_count == __bss_count)
    return 0;
  init_count = __bss_count;
  num_fds = 0;
  fsext_list = 0;

  /* Attach our hook to close all of the remaining open extensions. */
  __FSEXT_exit_hook = __FSEXT_close_all;
  return 1;
}

int
__FSEXT_alloc_fd(__FSEXT_Function *_function)
{
  static int null_dev_fd = -1;
  int fd;
  __dpmi_regs r = {0};

  if (init() || null_dev_fd == -1)
  {
    /*  This is our first time.  Open the NUL device, just this once.  */
    _put_path("nul");
    r.x.ax = 0x3d82;	/* open, no inherit, read/write */
    r.x.dx = __tb_offset;
    r.x.ds = __tb_segment;
    __dpmi_int(0x21, &r);
    null_dev_fd = (r.x.flags & 1) == 0 ? r.x.ax : -1;
  }

  if (null_dev_fd != -1)
  {
    /* To get a new handle, just dup the handle we got the first time.  This
       has the advantage of being independent of what their FILES= says.  */
    r.h.ah = 0x45;
    r.x.bx = null_dev_fd;
    __dpmi_int(0x21, &r);
  }

  if (r.x.flags & 1)
  {
    errno = __doserr_to_errno(r.x.ax);
    return -1;
  }

  fd = r.x.ax;
  __file_handle_set(fd, O_BINARY); /* so the JFT is expanded as needed */
  __FSEXT_set_function(fd, _function);
  return fd;
}

static int
grow_table(int _fd)
{
  /* Allocate table in chunks of chunk_sz */
  const int chunk_sz = 1<<8; /* 256 */

  init();

  if (_fd < 0)
    return 1;

  if (num_fds <= _fd)
  {
    __FSEXT_entry *temp;
    int old_fds = num_fds, i;

    num_fds = (_fd+chunk_sz) & ~(chunk_sz-1);
    temp = (__FSEXT_entry*)realloc(fsext_list, num_fds * sizeof(__FSEXT_entry));
    if (temp == 0)
    {
      /* Keep the current fsext_list, so that we can tidy the FSEXTs
	 up properly. */
      num_fds = old_fds;
      return 1;
    }

    fsext_list = temp;
    for (i=old_fds; i<num_fds; i++)
    {
      fsext_list[i].function = 0;
      fsext_list[i].data = 0;
    }
  }
  return 0;
}

int
__FSEXT_set_function(int _fd, __FSEXT_Function *_function)
{
  if (grow_table(_fd))
    return 1;
  fsext_list[_fd].function = _function;
  return 0;
}

__FSEXT_Function *
__FSEXT_get_function(int _fd)
{
  init();
  if (_fd < 0 || _fd >= num_fds)
    return 0;
  return fsext_list[_fd].function;
}

/*
   This is an exit helper routine.  This should be called *after* all of the
   other IO in libc has been closed -- that is, so that anything will opened
   with fopen can be fclose'd.

   Other note: *If* a fsext uses a fopen'd file, it gets in a race condition
   at exit.  That is, the file the fsext fopen'd may be closed before the
   fsext.  This is more of a bug with *bad* programs not good ones.
 */

static void
__FSEXT_close_all (void)
{
  int i;

  if (!fsext_list)
    return;
  __FSEXT_exit_hook=NULL;
  for (i = 0; i < num_fds; i++)
   if (fsext_list[i].function)
     _close(i);

  /* Free the table */
  free(fsext_list);
  fsext_list = NULL;
}

void *
__FSEXT_set_data(int _fd, void *_data)
{
  if (grow_table(_fd))
    return 0;
  fsext_list[_fd].data = _data;
  return _data;
}

void *
__FSEXT_get_data(int _fd)
{
  init();
  if (_fd < 0 || _fd >= num_fds)
    return 0;
  return fsext_list[_fd].data;
}
