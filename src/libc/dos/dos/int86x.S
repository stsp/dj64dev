/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
	.data
s_es:	.word	0
s_ds:	.word	0
s_fs:	.word	0
s_gs:	.word	0

	.text
	.globl	__int86x
__int86x:
	movl	16(%esp), %eax

	movl	(%eax), %ecx		/* Do both es & ds at same time */
	movl	%ecx, s_es
	movl	4(%eax), %ecx		/* Do both fs & gs at same time */
	movl	%ecx, s_fs

	jmp	int86_common

	.globl	__int86
__int86:
	movw	%ds, %ax
	movw	%ax, s_ds
	movw	%es, %ax
	movw	%ax, s_es
	movw	%fs, %ax
	movw	%ax, s_fs
	movw	%gs, %ax
	movw	%ax, s_gs

int86_common:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushf

	movl	8(%ebp),%eax
	movb	%al,int86_vec

	movl	12(%ebp),%eax
	movl	(%eax),%edi
	movl	4(%eax),%esi
	movl	8(%eax),%ebp
	movl	16(%eax),%ebx
	movl	20(%eax),%edx
	movl	24(%eax),%ecx
	movl	28(%eax),%eax

	pushl	%ds
	pushl	%es

	.byte	0x2e		/* CS: */
	push	s_ds
	popl	%ds
	.byte	0x2e
	push	s_es
	popl	%es
	.byte	0x2e
	push	s_fs
	popl	%fs
	.byte	0x2e
	push	s_gs
	popl	%gs

	.byte	0xcd
int86_vec:
	.byte	0x03

	popl	%es
	popl	%ds

	pushf
	pushl	%eax
	pushl	%ebp
	movl	%esp,%ebp
	addl	$28,%ebp
	movl	16(%ebp),%eax
	popl	8(%eax)			/* EBP */
	popl	28(%eax)		/* EAX */
	movl	%edi,(%eax)
	movl	%esi,4(%eax)
	movl	%ebx,16(%eax)
	movl	%edx,20(%eax)
	movl	%ecx,24(%eax)
	popl	%ebx			/* flags */
	movw	%bx,32(%eax)
	andl	$1,%ebx
	movl	%ebx,12(%eax)
	movl	28(%eax),%eax

	popf
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

#include <libc/asmdefs.h>
