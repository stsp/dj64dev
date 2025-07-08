/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <string.h>

#undef bcopy

void
bcopy(const void *a, void *b, size_t len)
{
  memmove(b, a, len);
}

//void __attribute__((alias("bcopy")))
//_bcopy(const void *a, void *b, size_t len);
