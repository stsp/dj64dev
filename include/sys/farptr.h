/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/* Copyright (c) 1995 DJ Delorie.  Permission granted to use for any
   purpose, provided this copyright remains attached and unmodified.

   THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

╔═══════════════════════════════════════════════════════════════════════╗
║		Far Pointer Simulation Functions			║
╚═══════════════════════════════════════════════════════════════════════╝

This file attempts to make up for the lack of a "far" keyword in GCC.
Although it doesn't provide access to far call APIs (like Windows), it
does allow you to do far pointer data access without the overhead of
movedata() or dosmemget/dosmemput().

You should *always* include this file when using these functions and
compile with optimization enabled.  They don't exist as normal functions
in any library, and they compile down to only a few opcodes when used
this way.  They are almost as fast as native pointer operations, and
about as fast as far pointers can get.

If you don't use optimization, this file becomes prototypes for
farptr.c, which generates real functions for these when not optimizing.
When optimizing, farptr.c compiles to nothing.

There are two types of functions here - standalone and invariant.  The
standalone functions take a selector and offset.  These are used when
you need only a few accesses, time isn't critical, or you don't know
what's in the %fs register.  The invariant ones don't take a selector,
they only take an offset.  These are used inside loops and in
time-critical accesses where the selector doesn't change.  To specify
the selector, use the farsetsel() function.  That selector is used for
all farns*() functions until changed.  You can use _fargetsel() if you
want to temporary change the selector with _farsetsel() and restore
it afterwards.

The farpoke* and farpeek* take selectors.

The farnspoke* and farnspeek* don't (note the `ns' for `no selector').

Warning: These routines all use the %fs register for their accesses.
GCC normally uses only %ds and %es, and libc functions (movedata,
dosmemget, dosmemput) use %gs.  Still, you should be careful about
assumptions concerning whether or not the value you put in %fs will be
preserved across calls to other functions.  If you guess wrong, your
program will crash.  Better safe than sorry.

*/

#ifndef __dj_include_sys_farptr_h_
#define __dj_include_sys_farptr_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

#include <sys/cdefs.h>

void _farpokeb(unsigned short, ULONG32, unsigned char);
void _farpokew(unsigned short, ULONG32, unsigned short);
void _farpokel(unsigned short, ULONG32, ULONG32);
unsigned char _farpeekb(unsigned short, ULONG32);
unsigned short _farpeekw(unsigned short, ULONG32);
ULONG32 _farpeekl(unsigned short, ULONG32);
void _farsetsel(unsigned short);
unsigned short _fargetsel(void);
void _farnspokeb(ULONG32, unsigned char);
void _farnspokew(ULONG32, unsigned short);
void _farnspokel(ULONG32, ULONG32);
unsigned char _farnspeekb(ULONG32);
unsigned short _farnspeekw(ULONG32);
ULONG32 _farnspeekl(ULONG32);

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_sys_farptr_h_ */
