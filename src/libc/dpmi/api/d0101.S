/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include "dpmidefs.h"

	FUNC(___dpmi_free_dos_memory)
	ENTER

	movl	ARG1, %edx
	DPMI(0x0101)

	xorl	%eax,%eax

	LEAVE

#include <libc/asmdefs.h>
