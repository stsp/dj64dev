/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#define USE_EBX
#include "dpmidefs.h"

	FUNC(___dpmi_resize_dos_memory)
	ENTER

	movl	ARG1, %edx
	movl	ARG2, %ebx
	DPMIce(0x0102)

	xorl	%eax,%eax
	RET

L_error:
	movl	ARG3, %ecx
	movzwl	%bx, %ebx
	movl	%ebx, (%ecx)
	movl	$-1, %eax

	LEAVE

#include <libc/asmdefs.h>
