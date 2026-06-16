/*
 *  dj64 - 64bit djgpp-compatible tool-chain
 *  Copyright (C) 2021-2024  @stsp
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define USE_EBX
#include <libc/asmdefs.h>

.macro farpoke sz,rg
	FUNC(__farpoke\sz)
	ENTER
	movl	ARG1,%eax
	movw	%ax,%fs
	movl	ARG2,%ebx
	movl	ARG3,%eax
	mov\sz	%\rg,%fs:(%ebx)
	_LEAVE
.endm

	farpoke b,al
	farpoke w,ax
	farpoke l,eax

.macro farpeek sz,rg
	FUNC(__farpeek\sz)
	ENTER
	movl	ARG1,%eax
	movw	%ax,%fs
	movl	ARG2,%ebx
	xorl	%eax,%eax
	mov\sz	%fs:(%ebx),%\rg
	_LEAVE
.endm

	farpeek b,al
	farpeek w,ax
	farpeek l,eax

	FUNC(__farsetsel)
	ENTER
	movl	ARG1,%eax
	movw	%ax,%fs
	_LEAVE

	FUNC(__fargetsel)
	ENTER
	xorl	%eax,%eax
	movw	%fs,%ax
	_LEAVE

.macro farnspoke sz,rg
	FUNC(__farnspoke\sz)
	ENTER
	movl	ARG1,%ebx
	movl	ARG2,%eax
	mov\sz	%\rg,%fs:(%ebx)
	_LEAVE
.endm

	farnspoke b,al
	farnspoke w,ax
	farnspoke l,eax

.macro farnspeek sz,rg
	FUNC(__farnspeek\sz)
	ENTER
	movl	ARG1,%ebx
	xorl	%eax,%eax
	mov\sz	%fs:(%ebx),%\rg
	_LEAVE
.endm

	farnspeek b,al
	farnspeek w,ax
	farnspeek l,eax
