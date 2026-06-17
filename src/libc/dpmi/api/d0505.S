/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#define USE_EBX
#define USE_ESI
#include "dpmidefs.h"

	.text

	FUNC(___dpmi_resize_linear_memory)
	ENTER

	movl	ARG1, %eax
	movl	(%eax), %esi
	movl	4(%eax), %ecx
	movl	ARG2, %edx
	andl	$1, %edx

	DPMI(0x0505)

	movl	ARG1, %edx
	movl	%ebx, 8(%edx)
	movl	%esi, (%edx)

	movzwl	%ax,%eax

	LEAVE

#include <libc/asmdefs.h>
