/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#define USE_EBX
#include "dpmidefs.h"

	FUNC(___dpmi_allocate_dos_memory)
	ENTER

	movl	ARG1, %ebx
	DPMIce(0x0100)

	movl	ARG2, %ecx
	movzwl	%dx, %edx
	movl	%edx, (%ecx)
	movzwl	%ax,%eax
	RET

L_error:
	movl	ARG2, %ecx
	cmpl	$0, %ecx
	je	L_no_ptr
	movzwl	%bx, %ebx
	movl	%ebx, (%ecx)
L_no_ptr:
	movl	$-1, %eax

	LEAVE

#include <libc/asmdefs.h>
