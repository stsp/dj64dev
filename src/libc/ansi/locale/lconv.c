/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <locale.h>
#include <limits.h>

static char ESTR[] = "";
static char DSTR[] = ".";

static struct lconv __lconv_ = {
  ESTR,
  DSTR,
  ESTR,
  ESTR,
  ESTR,
  ESTR,
  ESTR,
  ESTR,
  ESTR,
  ESTR,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX,
  CHAR_MAX
};

struct lconv *localeconv(void)
{
  return &__lconv_;
}
