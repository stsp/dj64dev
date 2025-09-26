#
# (C) Copyright 1992, ..., 2014 the "DOSEMU-Development-Team".
#
# for details see file COPYING in the DOSEMU distribution
#

#include "doshelpers.h"

.code16
.text
	.globl	_start16
_start16:

#define stack_size		0x100
#define stack			(stack_start + stack_size)
#define psp_size		0x100
#define DJSTUB_API_VER		5

	movw	$stack, %sp
				# first deallocate the unused memory, ES=PSP now
	movb	$0x4a, %ah
	movw	$(stack - _start16 + psp_size), %bx
	shrw	$4, %bx
	incw	%bx
	int	$0x21

	movb	$DOS_HELPER_ELFLOAD, %al
	movb	$ELFLOAD_PLUGIN_VERSION, %ah
	movw	$DJSTUB_API_VER, %dx
	int	$DOS_HELPER_INT /* doesn't return */
	movb	$0x4c, %ah
	int	$0x21

stack_start:
