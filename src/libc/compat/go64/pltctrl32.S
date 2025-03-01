#include <libc/asmdefs.h>

	FUNC(_pltctrl32)
	ENTER
	pushal
	movl ARG1, %ebx
.irpc i,76543210
	pushl \i*4(%ebx)
.endr
	movl %ebp, 8(%esp)  // keep ebp untouched
	popal
	pushl ARG3
	pushl ARG2
	call ctrl
	popl %ecx
	popl %ecx
	pushf
	pushal
	movl ARG1, %ebx
.irpc i,012345678
	popl \i*4(%ebx)
.endr
	popal
	LEAVE
