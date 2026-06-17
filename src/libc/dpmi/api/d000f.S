/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#define USE_EDI
#include "dpmidefs.h"

	FUNC(___dpmi_set_multiple_descriptors)
	ENTER

	movl	ARG1, %ecx
	movl	ARG2, %edi
	DPMIce(0x000f)

	movl	ARG1, %eax
	RET

L_error:
	movzwl	%cx, %eax
	negl	%eax

	LEAVE

#include <libc/asmdefs.h>
