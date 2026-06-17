/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/asmdefs.h>

	.text

	FUNC(__control87)
	ENTER

	pushl	%eax		/* make room on stack */
	fstcw	(%esp)
	fwait
	popl	%eax
	andl	$0xffff, %eax	/* OK;  we have the old value ready */

	movl	ARG2, %ecx
	notl	%ecx
	andl	%eax, %ecx	/* the bits we want to keep */

	movl	ARG2, %edx
	andl	ARG1, %edx	/* the bits we want to change */

	orl	%ecx, %edx	/* the new value */
	pushl	%edx
	fldcw	(%esp)
	popl	%edx

	LEAVE
