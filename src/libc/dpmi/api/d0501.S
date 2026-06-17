/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#define USE_EBX
#define USE_ESI
#define USE_EDI
#include "dpmidefs.h"

	.text

	FUNC(___dpmi_allocate_memory)
	ENTER

	movl	ARG1, %eax
	movw	4(%eax), %cx
	movw	6(%eax), %bx

	DPMI(0x0501)

	movl	ARG1, %edx
	movw	%cx, 8(%edx)
	movw	%bx, 10(%edx)
	movw	%di, (%edx)
	movw	%si, 2(%edx)

	xorl	%eax, %eax

	LEAVE

#include <libc/asmdefs.h>
