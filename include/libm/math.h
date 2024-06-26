/* Copyright (C) 2018 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2017 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2013 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/* Provided by Cygnus Support (jtc@cygnus.com) */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * from: @(#)fdlibm.h 5.1 93/09/24
 * $Id: math.h,v 1.14 2018/01/05 16:06:39 juan.guerrero Exp $
 */

#ifndef _MATH_H_
#define _MATH_H_

/*
 * ANSI/POSIX
 */
typedef int __int32_t;
typedef unsigned int __uint32_t;

union __dmath
{
  __uint32_t i[2];
  double d;
};

extern double __dj_huge_val;
#define HUGE_VAL (__dj_huge_val)

/*
 * XOPEN/SVID
 */
#if !defined(__STRICT_ANSI__) && !defined(_POSIX_SOURCE)
#define	M_E		2.7182818284590452354	/* e */
#define	M_LOG2E		1.4426950408889634074	/* log 2e */
#define	M_LOG10E	0.43429448190325182765	/* log 10e */
#define	M_LN2		0.693147180559945309417	/* log e2 */
#define	M_LN10		2.30258509299404568402	/* log e10 */
#define	M_PI		3.14159265358979323846	/* pi */
#define	M_TWOPI		6.28318530717958647692	/* 2*pi */
#define	M_PI_2		1.57079632679489661923	/* pi/2 */
#define	M_PI_4		0.78539816339744830962	/* pi/4 */
#define	M_3PI_4		2.3561944901923448370	/* 3/4 * pi */
#define	M_SQRTPI	1.77245385090551602792981 /* sqrt(pi) */
#define	M_1_PI		0.31830988618379067154	/* 1/pi */
#define	M_2_PI		0.63661977236758134308	/* 2/pi */
#define	M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#define	M_SQRT2		1.41421356237309504880	/* sqrt(2) */
#define	M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */
#define	M_LN2LO		1.9082149292705877000E-10 /* lower bits of log e2 */
#define	M_LN2HI		6.9314718036912381649E-1 /* log e2 */
#define	M_SQRT3   	1.73205080756887719000	/* sqrt(3) */
#define	M_IVLN10	0.43429448190325182765 /* 1 / log(10) */
#define	M_LOG2_E	0.693147180559945309417
#define	M_INVLN2	1.4426950408889633870E0  /* 1 / log e2 */

extern int signgam;

enum __fdlibm_version
{
  __fdlibm_ieee = -1,
  __fdlibm_svid,
  __fdlibm_xopen,
  __fdlibm_posix
};

#define _LIB_VERSION_TYPE enum __fdlibm_version
#define _LIB_VERSION __fdlib_version

/* if global variable _LIB_VERSION is not desirable, one may
 * change the following to be a constant by:
 *	#define _LIB_VERSION_TYPE const enum version
 * In that case, after one initializes the value _LIB_VERSION (see
 * s_lib_version.c) during compile time, it cannot be modified
 * in the middle of a program
 */
extern  _LIB_VERSION_TYPE  _LIB_VERSION;

#define _IEEE_  __fdlibm_ieee
#define _SVID_  __fdlibm_svid
#define _XOPEN_ __fdlibm_xopen
#define _POSIX_ __fdlibm_posix

/* The exception structure passed to the matherr routine.  */

#ifndef __cplusplus
struct exception
{
  int type;
  const char *name;
  double arg1;
  double arg2;
  double retval;
  int err;
};
#endif

/*
 * set X_TLOSS = pi*2**52, which is possibly defined in <values.h>
 * (one may replace the following line by "#include <values.h>")
 */

#define X_TLOSS		1.41484755040568800000e+16

#define	DOMAIN		1
#define	SING		2
#define	OVERFLOW	3
#define	UNDERFLOW	4
#define	TLOSS		5
#define	PLOSS		6

#endif /* !__STRICT_ANSI__ && !_POSIX_SOURCE */


#include <sys/cdefs.h>
__BEGIN_DECLS

/*
 * ANSI/POSIX
 */
extern double acos __P((double));
extern double asin __P((double));
extern double atan __P((double));
extern double atan2 __P((double, double));
extern double cos __P((double));
extern double sin __P((double));
extern double tan __P((double));

extern double cosh __P((double));
extern double sinh __P((double));
extern double tanh __P((double));

extern double exp __P((double));
extern double frexp __P((double, int *));
extern double ldexp __P((double, int));
extern double log __P((double));
extern double log10 __P((double));
extern double modf __P((double, double *));

extern double pow __P((double, double));
extern double sqrt __P((double));

extern double ceil __P((double));
extern double fabs __P((double));
extern double floor __P((double));
extern double fmod __P((double, double));

extern long double frexpl __P((long double, int *));

#if !defined(__STRICT_ANSI__) || defined(__cplusplus) || \
    defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

/* ISO C99 types and macros. */
extern long long int llrintf __P((float));
extern long long int llrint __P((double));
extern long long int llrintl __P((long double));
extern long int lrintf __P((float));
extern long int lrint __P((double));
extern long int lrintl __P((long double));
extern long long int llroundf __P((float));
extern long long int llround __P((double));
extern long long int llroundl __P((long double));
extern long int lroundf __P((float));
extern long int lround __P((double));
extern long int lroundl __P((long double));
extern float roundf __P((float));
extern double round __P((double));
extern long double roundl __P((long double));
extern float truncf __P((float));
extern double trunc __P((double));
extern long double truncl __P((long double));
extern int isinfl __P((long double));
extern int isnanl __P((long double));
extern int finitel __P((long double));
#endif /* !defined (__STRICT_ANSI__) || defined(__cplusplus)
          || defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L */

#if !defined(__STRICT_ANSI__) && !defined(_POSIX_SOURCE)
extern double erf __P((double));
extern double erfc __P((double));
extern double gamma __P((double));
extern double hypot __P((double, double));
extern double infinity __P((void));
extern int isinf __P((double));
extern int isnan __P((double));
extern int finite __P((double));
extern double j0 __P((double));
extern double j1 __P((double));
extern double jn __P((int, double));
extern double lgamma __P((double));
extern double nan __P((const char *));
extern double y0 __P((double));
extern double y1 __P((double));
extern double yn __P((int, double));
extern double log2 __P((double));

#if !defined(_XOPEN_SOURCE)
extern double acosh __P((double));
extern double asinh __P((double));
extern double atanh __P((double));
extern double cbrt __P((double));
extern double exp10 __P((double));
extern double exp2 __P((double));
extern double log1p __P((double));
extern double logb __P((double));
extern long double modfl __P((long double, long double *));
extern double nextafter __P((double, double));
extern double pow10 __P((double));
extern double pow2 __P((double));
extern double powi __P((double, int));
extern void   sincos __P((double, double *, double *));
extern double remainder __P((double, double));
extern double scalb __P((double, double));

#ifndef __cplusplus
extern int matherr __P((struct exception *));
#endif

extern long double rintl __P((long double));

/*
 * IEEE Test Vector
 */
extern double significand __P((double));

/*
 * Functions callable from C, intended to support IEEE arithmetic.
 */
extern double copysign __P((double, double));
extern int ilogb __P((double));
extern double rint __P((double));
extern double scalbn __P((double, int));

/*
 * BSD math library entry points
 */
extern double drem __P((double, double));
extern double expm1 __P((double));
extern double log1p __P((double));

/*
 * Reentrant version of gamma & lgamma; passes signgam back by reference
 * as the second argument; user must allocate space for signgam.
 */
extern double gamma_r __P((double, int *));
extern double lgamma_r __P((double, int *));


/* float versions of ANSI/POSIX functions */
extern float acosf __P((float));
extern float asinf __P((float));
extern float atanf __P((float));
extern float atan2f __P((float, float));
extern float cosf __P((float));
extern float sinf __P((float));
extern float tanf __P((float));

extern float coshf __P((float));
extern float sinhf __P((float));
extern float tanhf __P((float));

extern float expf __P((float));
extern float frexpf __P((float, int *));
extern float ldexpf __P((float, int));
extern float logf __P((float));
extern float log10f __P((float));
extern float log2f __P((float));
extern float modff __P((float, float *));

extern float powf __P((float, float));
extern float sqrtf __P((float));

extern float ceilf __P((float));
extern float fabsf __P((float));
extern float floorf __P((float));
extern float fmodf __P((float, float));

extern float erff __P((float));
extern float erfcf __P((float));
extern float gammaf __P((float));
extern float hypotf __P((float, float));
extern float infinityf __P((void));
extern int isinff __P((float));
extern int isnanf __P((float));
extern int finitef __P((float));
extern float j0f __P((float));
extern float j1f __P((float));
extern float jnf __P((int, float));
extern float lgammaf __P((float));
extern float nanf __P((const char *));
extern float y0f __P((float));
extern float y1f __P((float));
extern float ynf __P((int, float));

extern float acoshf __P((float));
extern float asinhf __P((float));
extern float atanhf __P((float));
extern float cbrtf __P((float));
extern float logbf __P((float));
extern float nextafterf __P((float, float));
extern float remainderf __P((float, float));
extern float scalbf __P((float, float));

/*
 * float version of IEEE Test Vector
 */
extern float significandf __P((float));

/*
 * Float versions of functions callable from C, intended to support
 * IEEE arithmetic.
 */
extern float copysignf __P((float, float));
extern int ilogbf __P((float));
extern float rintf __P((float));
extern float scalbnf __P((float, int));

/*
 * float versions of BSD math library entry points
 */
extern float dremf __P((float, float));
extern float expm1f __P((float));
extern float log1pf __P((float));

/*
 * Float versions of reentrant version of gamma & lgamma; passes
 * signgam back by reference as the second argument; user must
 * allocate space for signgam.
 */
extern float gammaf_r __P((float, int *));
extern float lgammaf_r __P((float, int *));

#endif /* !_XOPEN_SOURCE */
#endif /* !__STRICT_ANSI__ && !_POSIX_SOURCE */

__END_DECLS

#endif /* _MATH_H_ */
