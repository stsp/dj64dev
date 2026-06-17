/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dpmi.h>

#include <libc/dosio.h>
#include <libc/bss.h>
#include <libc/djctx.h>

#define init_fh { \
  O_TEXT, \
  O_TEXT, \
  O_TEXT, \
  O_BINARY, \
  O_BINARY, \
}

struct fh_state {
  char init_file_handle_modes[20];
  int dosio_bss_count;
  size_t count;	/* DOS default limit */
  char *__file_handle_modes;
};

static char init_file_handle_mo[20] = init_fh;
char *__file_handle_modes = init_file_handle_mo;

#if USE64
static struct fh_state *fhs;

static const struct fh_state fhinit =
{
  .init_file_handle_modes = init_fh,
  .dosio_bss_count = -1,
  .count = 20,
};

static void fhs_pre(void)
{
#define _SV(x) fhs->x = x
  _SV(__file_handle_modes);
}
static void fhs_post(void)
{
#define _RS(x) x = fhs->x
  _RS(__file_handle_modes);
}
static void fhs_init(struct fh_state *st)
{
  st->__file_handle_modes = st->init_file_handle_modes;
}
DJ64_DEFINE_SWAPPABLE_CONTEXT3(fh_state, fhs, fhinit,
    fhs_pre(), fhs_post(), fhs_init);

#define init_file_handle_modes fhs->init_file_handle_modes
#define dosio_bss_count fhs->dosio_bss_count
#define count fhs->count
#else
static struct fh_state fhs;
#define init_file_handle_modes fhs.init_file_handle_modes
#define dosio_bss_count fhs.dosio_bss_count
#define count fhs.count
#endif

void
__file_handle_set(int fd, int mode)
{
  __dpmi_regs r = {0};

  if (dosio_bss_count != __bss_count)
  {
    dosio_bss_count = __bss_count;
    count = 20;
  }

  /* Check for bogus values */
  if (fd < 0)
    return;

  /* See if we need to expand the tables.  Check this BEFORE it might fail,
     so that when we hit the count'th request, we've already up'd it. */
  if ((size_t)fd >= (count-1) && count < 255)
  {
    int oldcount = count;
    count = 255;

    __file_handle_modes = (char *)malloc(count * sizeof(*__file_handle_modes));
    memcpy(__file_handle_modes, init_file_handle_modes, sizeof(init_file_handle_modes));
    memset(__file_handle_modes + oldcount, 0, (count-oldcount)*sizeof(__file_handle_modes[0]));

    /* Tell DOS to allow more */
    r.h.ah = 0x67;
    r.x.bx = count;
    __dpmi_int(0x21, &r);
  }

  /* Fill in the value */
  __file_handle_modes[fd] = mode;
}
