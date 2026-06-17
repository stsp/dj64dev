/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#define USE_EBX
#define USE_EDI
#include "dpmidefs.h"

	.comm	___dpmi_int_ss, 2
	.comm	___dpmi_int_sp, 2
	.comm	___dpmi_int_flags, 2
	.lcomm	dpmi_busy, 1

	.text

	FUNC(___dpmi_int)
	ENTER

	pushl	%es
	movw	___djgpp_ds_alias, %ax
	movw	%ax, %es

	movl	ARG1, %ebx
	movl	$0, %ecx
	movl	ARG2, %edi
	incb	dpmi_busy

	cmpb	$1, dpmi_busy
	jne	L_is_busy

	movw	___dpmi_int_flags, %ax
	movw	%ax, 0x20(%edi)	/* r.x.flags */
	movw	___dpmi_int_sp, %ax
	movw	%ax, 0x2e(%edi)	/* r.x.sp */
	movw	___dpmi_int_ss, %ax
	movw	%ax, 0x30(%edi)	/* r.x.ss */
	jmp	L_not_busy

L_is_busy:
	movw	$0, 0x20(%edi)	/* r.x.flags */
	movw	$0, 0x2e(%edi)	/* r.x.sp */
	movw	$0, 0x30(%edi)	/* r.x.ss */

L_not_busy:
	DPMI(0x0300)
	xorl	%eax,%eax

	LEAVEP(decb dpmi_busy; popl %es)

#include <libc/asmdefs.h>
