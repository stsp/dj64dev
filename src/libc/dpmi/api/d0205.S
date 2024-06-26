/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#define USE_EBX
#include "dpmidefs.h"

	.text

	FUNC(___dpmi_set_protected_mode_interrupt_vector)
	ENTER

	movb	ARG1, %bl
	movl	ARG2, %eax
	movl	(%eax), %edx
	movw	4(%eax), %cx
	DPMI(0x0205)
	xorl	%eax,%eax

	LEAVE

#include <libc/asmdefs.h>
