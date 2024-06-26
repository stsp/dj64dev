/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_errno_h_
#define __dj_include_errno_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "libc/asmobj.h"

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#define EDOM		1
#define ERANGE		2

#define errno __dj_errno
EXTERN ASM_N(int, __dj_errno);

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#define EILSEQ		41

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#if !defined(__STRICT_ANSI__) || (defined(__cplusplus) && ( \
   (__GNUC__ == 4 && (__GNUC_MINOR__ > 2 && __GNUC_MINOR < 7) && \
   defined(__GXX_EXPERIMENTAL_CXX0X__)) || (__cplusplus >= 201103L)))

#define E2BIG		3
#define EACCES		4
#define EAGAIN		5
#define EBADF		6
#define EBUSY		7
#define ECHILD		8
#define EDEADLK		9
#define EEXIST		10
#define EFAULT		11
#define EFBIG		12
#define EINTR		13
#define EINVAL		14
#define EIO		15
#define EISDIR		16
#define EMFILE		17
#define EMLINK		18
#define ENAMETOOLONG	19
#define ENFILE		20
#define ENODEV		21
#define ENOENT		22
#define ENOEXEC		23
#define ENOLCK		24
#define ENOMEM		25
#define ENOSPC		26
#define ENOSYS		27
#define ENOTDIR		28
#define ENOTEMPTY	29
#define ENOTTY		30
#define ENXIO		31
#define EPERM		32
#define EPIPE		33
#define EROFS		34
#define ESPIPE		35
#define ESRCH		36
#define EXDEV		37

#ifndef _POSIX_SOURCE

#define ENMFILE		38
#define ELOOP           39
#define EOVERFLOW	40
/* EILSEQ		41 */

extern char *		sys_errlist[];
extern int		sys_nerr;
extern const char *	__sys_errlist[];
extern int		__sys_nerr;
extern int		_doserrno;

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ || ... */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_errno_h_ */
