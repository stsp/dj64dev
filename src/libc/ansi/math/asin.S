/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
#include <libc/asmdefs.h>

NaN:
	.long	0xFFC00000

	.globl	_asin
_asin:
/* The commonly-used formula

	   asin(x) = atan2(x, sqrt(1.-x*x))

	is not a bad way to compute the arcsine, but there are subtle errors for
	x near 1 (or -1) caused by inaccuracies in forming x*x.	 However, the
	argument to the square root can be computed accurately even for x near 1
	by the alternate form

	   asin(x) = atan2(x, sqrt((1.+x)*(1.-x)))

	which is used here. */

	movl	8(%esp), %eax		/* is x >= 1? */
	andl	$0x7FF00000, %eax	/* |x| */
	cmpl	$0x3FF00000, %eax
	jg	badarg			/* |x| >= 2 or x = inf or NaN */
	je	onetotwo		/* 1 <= |x| < 2 */

argok:
	fldl	4(%esp)			/* x		  */
	fld	%st(0)			/* x	x	  */
	fld	%st(0)			/* x	x   x	  */
	fld1				/* 1	x   x	x */
	faddp	%st, %st(2)		/* x	1+x x	  */
	fld1				/* 1	x   1+x	  */
	/* Should be fsubrp %st, %st(1) (gas bug) */
	.byte	0xDE, 0xE1		/* 1-x	1+x x	  */
	fmulp				/* 1-x2 x	  */
	fsqrt
	fpatan
	ret

onetotwo:
	movl	8(%esp), %eax		/* is x = +/-1? */
	andl	$0x7FFFFFFF, %eax	/* |x| */
	cmpl	$0x3FF00000, %eax
	jne	badarg
	movl	4(%esp), %eax
	testl	%eax, %eax
	jz	argok

badarg:
	movl	$1, _errno
	flds	NaN
	ret
