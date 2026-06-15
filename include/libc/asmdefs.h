/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_libc_asmdefs_h__
#define __dj_include_libc_asmdefs_h__

	.file	__BASE_FILE__

#ifdef USE_EBX
#define PUSHL_EBX	pushl %ebx;
#define POPL_EBX	popl %ebx;
#else
#define PUSHL_EBX
#define POPL_EBX
#endif

#ifdef USE_ESI
#define PUSHL_ESI	pushl %esi;
#define POPL_ESI	popl %esi;
#else
#define PUSHL_ESI
#define POPL_ESI
#endif

#ifdef USE_EDI
#define PUSHL_EDI	pushl %edi;
#define POPL_EDI	popl %edi;
#else
#define PUSHL_EDI
#define POPL_EDI
#endif

#define USE_FAR_CALL USE64
#if USE_FAR_CALL
#define OFF 4
#define RET_I lretl
#else
#define OFF 0
#define RET_I ret
#endif

#define FUNC(x)		.globl x; x:

#define ENTER		pushl %ebp; movl %esp,%ebp; PUSHL_EBX PUSHL_ESI PUSHL_EDI

#define _LEAVE		POPL_EDI POPL_ESI POPL_EBX movl %ebp,%esp; popl %ebp; RET_I
#define LEAVE		L_leave: _LEAVE
#define LEAVEP(x)	L_leave: x; POPL_EDI POPL_ESI POPL_EBX movl %ebp,%esp; popl %ebp; RET_I

#define RET		jmp L_leave

#define ARG1		8+OFF(%ebp)
#define ARG1h		10+OFF(%ebp)
#define ARG2		12+OFF(%ebp)
#define ARG2h		14+OFF(%ebp)
#define ARG3		16+OFF(%ebp)
#define ARG4		20+OFF(%ebp)
#define ARG5		24+OFF(%ebp)
#define ARG6		28+OFF(%ebp)
#define ARG7		32+OFF(%ebp)
#define ARG8		36+OFF(%ebp)

#define _errno ___dj_errno

#ifdef __ELF__
.section .note.GNU-stack,"",%progbits
.previous
#endif

#endif /* __dj_include_libc_asmdefs_h__ */
