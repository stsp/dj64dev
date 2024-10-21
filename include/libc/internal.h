/* Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_libc_internal_h__
#define __dj_include_libc_internal_h__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE
#include "libc/asmobj.h"

typedef int (main_t)(int, char **, char **);
void __crt1_startup(main_t *main);
void _npxsetup(char *argv0);
void __emu387_exception_handler(void);
void __djgpp_exception_processor(void);
void __djgpp_exception_setup(void);
void __djgpp_signal_setup(void);
void __register_frame_info(const void *begin, const void *object);
void __deregister_frame_info(const void *begin);
void _crt0_init_mcount(void);	/* For profiling */
#if 0
typedef void (*FUNC)(void);
extern FUNC djgpp_first_ctor;
extern FUNC djgpp_last_ctor;
extern FUNC djgpp_first_dtor;
extern FUNC djgpp_last_dtor;
#endif
void init_sys_siglist(void);
void init_confstr(void);

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* __dj_include_libc_internal_h__ */
