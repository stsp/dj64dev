/* Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_sys_nearptr_h_
#define __dj_include_sys_nearptr_h_

#include "libc/asmobj.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

/* Functions to enable "near" pointer access to DOS memory under DPMI
   CW Sandmann 7-95  NO WARRANTY: WARNING, since these functions disable
   memory protection, they MAY DESTROY EVERYTHING ON YOUR COMPUTER! */

int __djgpp_nearptr_enable(void);	/* Returns 0 if feature not avail */
void __djgpp_nearptr_disable(void);	/* Enables protection */
EXTERN ASM(int, __djgpp_selector_limit);	/* Limit on CS and on DS if prot */
EXTERN ASM(unsigned, __djgpp_base_address);	/* Used in calculation below */

#define __djgpp_conventional_base (-__djgpp_base_address)

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_sys_nearptr_h_ */
