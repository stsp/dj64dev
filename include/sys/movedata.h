/* Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_sys_movedata_h_
#define __dj_include_sys_movedata_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

#include <sys/djtypes.h>
#include <stdint.h>
#ifndef IN_ASMOBJ
#include <sys/fmemcpy.h>
#define DP(s, o) (__dpmi_paddr){ .selector = s, .offset32 = o, }
#endif

#ifndef _SIZE_T
__DJ_size_t
#define _SIZE_T
#endif

/* This header is intended to be included only by other
   headers, like <go32.h> and <string.h>.  You may
   include this directly, but it will be non-portable. */

/* These lengths are in bytes, optimized for speed */
void dosmemget(unsigned long _offset, size_t _length, void *_buffer);
void dosmemput(const void *_buffer, size_t _length, unsigned long _offset);

/* The lengths here are in TRANSFERS, not bytes! */
void _dosmemgetb(unsigned long _offset, size_t _xfers, void *_buffer);
void _dosmemgetw(unsigned long _offset, size_t _xfers, void *_buffer);
void _dosmemgetl(unsigned long _offset, size_t _xfers, void *_buffer);
void _dosmemputb(const void *_buffer, size_t _xfers, unsigned long _offset);
void _dosmemputw(const void *_buffer, size_t _xfers, unsigned long _offset);
void _dosmemputl(const void *_buffer, size_t _xfers, unsigned long _offset);

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_sys_movedata_h_ */
