#include <libc/asmdefs.h>

	FUNC(_pltcall32)
	ENTER
	pushal
	movl ARG1, %ebx
.irpc i,76543210
	pushl \i*4(%ebx)
.endr
	movl %ebp, 8(%esp)  // keep ebp untouched
	popal
	movl __plt_handle, %eax
	lcalll *ARG2
	pushf
	pushal
	movl ARG1, %ebx
.irpc i,012345678
	popl \i*4(%ebx)
.endr
	popal
	LEAVE
