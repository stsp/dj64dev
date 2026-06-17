/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#define USE_ESI
#define USE_EDI
#include "dpmidefs.h"

	.text

	FUNC(___dpmi_allocate_real_mode_callback)
	ENTER

	pushl	%es
	movw	___djgpp_ds_alias, %ax	/* from exceptn.S */
	movw	%ax, %es

	pushl	%ds
	pushl	%cs
	popl	%ds

	movl	ARG1, %esi
	movl	ARG2, %edi

	movw	$0x0303,%ax
	int	$0x31
	popl	%ds
	popl	%es
	jnc	L_noerror
	movw	%ax, ___dpmi_error
	movl	$-1,%eax
	jmp	L_leave

L_noerror:
	movl	ARG3, %eax
	movw	%dx, (%eax)
	movw	%cx, 2(%eax)
	xorl	%eax, %eax

	LEAVE

#include <libc/asmdefs.h>
