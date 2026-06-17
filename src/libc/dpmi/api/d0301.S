/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#define USE_EBX
#define USE_EDI
#include "dpmidefs.h"

	.text

	FUNC(___dpmi_simulate_real_mode_procedure_retf)
	ENTER

	pushl	%es
	movw	___djgpp_ds_alias, %ax
	movw	%ax, %es

	movl	$0, %ebx
	movl	$0, %ecx
	movl	ARG1, %edi
	DPMI(0x0301)
	xorl	%eax, %eax

	LEAVEP(popl %es)

#include <libc/asmdefs.h>
