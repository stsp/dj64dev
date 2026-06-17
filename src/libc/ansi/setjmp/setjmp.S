/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/* This is file SETJMP.S */
/*
** Copyright (C) 1993 DJ Delorie, 334 North Rd, Deerfield NH 03037-1110
**
** This file is distributed under the terms listed in the document
** "copying.dj", available from DJ Delorie at the address above.
** A copy of "copying.dj" should accompany this file; if not, a copy
** should be available from where this file was obtained.  This file
** may not be distributed without a verbatim copy of "copying.dj".
**
** This file is distributed WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*
**	jmp_buf:
**	 eax ebx ecx edx esi edi ebp esp eip fl cs ds es fs gs ss
**	 0   4   8   12  16  20  24  28  32  36 40 42 44 46 48 50
*/

	.globl	_setjmp /* jmp_buf */
_setjmp:
	pushl	%ebp
	movl	%esp,%ebp

	pushl	%edi
	movl	8(%ebp),%edi

	movl	%eax, (%edi)
	movl	%ebx,4(%edi)
	movl	%ecx,8(%edi)
	movl	%edx,12(%edi)
	movl	%esi,16(%edi)

	movl	-4(%ebp),%eax
	movl	%eax,20(%edi)

	movl	(%ebp),%eax
	movl	%eax,24(%edi)

	movl	%esp,%eax
	addl	$12,%eax
	movl	%eax,28(%edi)

	movl	4(%ebp),%eax
	movl	%eax,32(%edi)

	pushfl
	popl	36(%edi)

	movw	%cs, 40(%edi)
	movw	%ds, 42(%edi)
	movw	%es, 44(%edi)
	movw	%fs, 46(%edi)
	movw	%gs, 48(%edi)
	movw	%ss, 50(%edi)

	movl	___djgpp_exception_state_ptr, %eax
	movl	%eax, 60(%edi)

	popl	%edi
	xorl	%eax,%eax
	popl	%ebp
	ret

#include <libc/asmdefs.h>
