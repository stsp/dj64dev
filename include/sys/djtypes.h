/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#ifndef __DJ_sys_djtypes_h_
#define __DJ_sys_djtypes_h_

#define __DJ_clock_t	typedef int clock_t;
#define __DJ_gid_t	typedef int gid_t;
#define __DJ_off_t	typedef int off_t;
#define __DJ_off64_t	__extension__ typedef long long off64_t;
#define __DJ_offset_t	__extension__ typedef long long offset_t;
#define __DJ_pid_t	typedef int pid_t;
#ifdef __LP64__
#define __DJ_size_t	typedef long unsigned int size_t;
#define __DJ_ssize_t	typedef long signed int ssize_t;
#else
#define __DJ_size_t	typedef unsigned int size_t;
#define __DJ_ssize_t	typedef signed int ssize_t;
#endif
#define __DJ_time_t	typedef unsigned int time_t;
#define __DJ_uid_t	typedef int uid_t;

/* For GCC 3.00 or later we use its builtin va_list.                       */
#if __GNUC__ >= 3
#define __DJ_va_list    typedef __builtin_va_list va_list;
#else
#define __DJ_va_list	typedef void *va_list;
#endif

#if defined(__cplusplus) && ( (__GNUC_MINOR__ >= 8 && __GNUC__ == 2 ) || __GNUC__ >= 3 )
/* wchar_t is now a keyword in C++ */
#define __DJ_wchar_t
#else
/* but remains a typedef in C */
#define __DJ_wchar_t    typedef __WCHAR_TYPE__ wchar_t;
#endif

#define __DJ_wint_t     typedef int wint_t;

/* Define internal type to be used for avoiding strict
   aliasing warnings */
#if (__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ > 3))
typedef short __attribute__((__may_alias__))                __dj_short_a;
typedef int __attribute__((__may_alias__))                  __dj_int_a;
typedef long __attribute__((__may_alias__))                 __dj_long_a;
typedef long long __attribute__((__may_alias__))            __dj_long_long_a;
typedef unsigned short __attribute__((__may_alias__))       __dj_unsigned_short_a;
typedef unsigned int __attribute__((__may_alias__))         __dj_unsigned_int_a;
typedef unsigned long __attribute__((__may_alias__))        __dj_unsigned_long_a;
typedef unsigned long long __attribute__((__may_alias__))   __dj_unsigned_long_long_a;
typedef float __attribute__((__may_alias__))                __dj_float_a;
typedef double __attribute__((__may_alias__))               __dj_double_a;
typedef long double __attribute__((__may_alias__))          __dj_long_double_a;
#else
typedef short                __dj_short_a;
typedef int                  __dj_int_a;
typedef long                 __dj_long_a;
typedef long long            __dj_long_long_a;
typedef unsigned short       __dj_unsigned_short_a;
typedef unsigned int         __dj_unsigned_int_a;
typedef unsigned long        __dj_unsigned_long_a;
typedef unsigned long long   __dj_unsigned_long_long_a;
typedef float                __dj_float_a;
typedef double               __dj_double_a;
typedef long double          __dj_long_double_a;
#endif

#endif
