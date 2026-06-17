/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#define USE_EBX
#define USE_ESI
#define USE_EDI
#include "dpmidefs.h"

	.text

	FUNC(___dpmi_physical_address_mapping)
	ENTER

	movl	ARG1, %eax
	movw	8(%eax), %cx
	movw	10(%eax), %bx
	movw	4(%eax), %di
	movw	6(%eax), %si

	DPMI(0x0800)

	movl	ARG1, %edx
	movw	%cx, 8(%edx)
	movw	%bx, 10(%edx)

	xorl	%eax,%eax

	LEAVE

#include <libc/asmdefs.h>
