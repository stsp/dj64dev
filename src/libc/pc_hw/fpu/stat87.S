/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/asmdefs.h>

	.text

	FUNC(__status87)

	fstsw	%ax
	movzwl	%ax, %eax
	RET_I
