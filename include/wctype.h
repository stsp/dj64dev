/* Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_wctype_h_
#define __dj_include_wctype_h_

/* Bare bones header to satisfy SGI STL's basic_string<> */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#include <sys/djtypes.h>

#ifndef _WCHAR_T
__DJ_wchar_t
#define _WCHAR_T
#endif
#ifndef _WINT_T
__DJ_wint_t
#define _WINT_T
#endif

#ifndef WEOF
#define WEOF ((wint_t)(-1))
#endif

/* Implementation defined types */
typedef unsigned short wctype_t;
typedef const unsigned char *wctrans_t;

int       iswprint(wint_t);

int       iswalnum(wint_t);
int       iswalpha(wint_t);
int       iswdigit(wint_t);

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_wctype_h_ */

