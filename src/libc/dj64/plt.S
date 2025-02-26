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

#include <libc/asmdefs.h>
#include "stubinfo.h"
#include "plt.h"
.include "plt_defs.inc"

.bss

__plt_open: .quad 0
PLT_OPEN_CMD = 0
PLT_CLOSE_CMD = 1
__plt_call: .quad 0
__plt_ctrl: .quad 0
.global __plt_handle
__plt_handle: .long 0

sel: .word 0
SHM_REQ_LEN = 0
SHM_RET_LEN = 4
SHM_HANDLE = 8
SHM_LINEAR = 0xc
SHM_NAME = 0x10
SHM_FLAGS = 0x16
SHM_HANDLE2 = 0x18
shm_block: .fill 0x1c, 1, 0
shm_block2: .fill 0x1c, 1, 0
handle: .long 0
maddr: .long 0
SHM_NOEXEC = 1
SHM_EXCL = 2
SHM_NEW_NS = 4
SHM_NS = 8
SHM_FLG_MASK = (SHM_NOEXEC | SHM_EXCL | SHM_NEW_NS | SHM_NS)

.data

dos_nm: .asciz "MS-DOS"
ext_nm: .asciz "DJ64"
err_str: .asciz "dj64: load failure\r\n$"
err2_str: .asciz "dj64: missing payload\r\n$"

.text

.macro asmcfunc_n nm,num
    .global _\nm
    _\nm:
    pushl %ebp
    movl %esp, %ebp
    pushal
    movl %esp, %edx
    movl $\num, %ecx
    movl $AUX_CORE, %ebx
    call_plt
    popal
    popl %ebp
    ret
.endm

.include "plt.inc"

uplt_init:
    pushl __plt_handle
    pushl __plt_call + 4
    pushl __plt_call
    call *%fs:STUBINFO_UENTRY
    addl $12, %esp
    ret

.global plt_init
plt_init:
    pushl %es
    /* check for extender, used for file I/O */
    movl $0x168a, %eax
    movl $dos_nm, %esi
    int $0x2f
    jc 2f
    /* check for DJ64 */
    movl $0xa00, %eax
    movl $ext_nm, %esi
    int $0x31
    jc 2f
    orb %al, %al
    jnz 2f
    movl %es, %eax
    movl %eax, __plt_open + 4
    movl %edi, __plt_open

.macro mk_shm b,offs,size,name,err,zerr,flg,h2
    movl %fs:\size, %ecx
    orl %ecx, %ecx
    jz \zerr
    xorb %al, %al
    cmpb %fs:\name, %al
    je \err
    movl %ecx, \b + SHM_REQ_LEN
    movw %fs, \b + SHM_NAME + 4
    movl $\name, \b + SHM_NAME
    movzbw %fs:\flg, %ax
    andb $SHM_FLG_MASK, %al
    movw %ax, \b + SHM_FLAGS
    movl \h2, %eax
    movl %eax, \b + SHM_HANDLE2
    /* alloc mem */
    pushw %ds
    popw %es
    movl $\b, %edi
    movl $0xd00, %eax
    int $0x31
    jc \err
    /* alloc desc */
    movl $0, %eax
    movl $1, %ecx
    int $0x31
    movw %ax, sel
    /* set base */
    movw %ax, %bx
    movw \b + SHM_LINEAR + 2, %cx
    movw \b + SHM_LINEAR, %dx
    movl $7, %eax
    int $0x31
    /* set limit */
    movw sel, %bx
    movw %fs:\size, %dx
    decw %dx
    movw %fs:\size + 2, %cx
    movl $8, %eax
    int $0x31
    /* seek fd */
    movl %fs:STUBINFO_SELF_FD, %ebx
    movzwl %fs:\offs, %edx
    movzwl %fs:\offs + 2, %ecx
    movl $0x4200, %eax
    int $0x21
    /* read payload */
    pushl %ds
    movw sel, %ds
    movl %fs:STUBINFO_SELF_FD, %ebx
    movl %fs:\size, %ecx
    movl $0, %edx
    movl $0x3f00, %eax
    int $0x21
    popl %ds
    jc 2f
    /* unmap shm */
    movw \b + SHM_LINEAR + 2, %bx
    movw \b + SHM_LINEAR, %cx
    movl $0x801, %eax
    int $0x31
.endm

    mk_shm shm_block,STUBINFO_PAYLOAD_OFFS,STUBINFO_PAYLOAD_SIZE,STUBINFO_ARGV0,2f,321f,STUBINFO_FLAGS,$0
    mk_shm shm_block2,STUBINFO_PAYLOAD2_OFFS,STUBINFO_PAYLOAD2_SIZE,STUBINFO_PAYLOAD2_NAME,5f,5f,STUBINFO_FLAGS+1,shm_block+SHM_HANDLE

5:
    // now open lib
    movl $0, %eax
    movl $PLT_OPEN_CMD, %ebx
    movw shm_block + SHM_HANDLE, %di
    movw shm_block + SHM_HANDLE + 2, %si
    movzbl %fs:STUBINFO_FLAGS, %ecx
    andl $~SHM_FLG_MASK, %ecx
    lcalll *__plt_open
    jc 2f
    movl %eax, __plt_handle
    movl %es, %eax
    movl %eax, __plt_call + 4
    movl %edi, __plt_call
    movl %eax, __plt_ctrl + 4
    movl %esi, __plt_ctrl
    movl %fs:STUBINFO_FLAGS, %ecx
    testw $0x4080, %cx
    jnz 111f
    call uplt_init
111:
    movl %fs:STUBINFO_SELF_SIZE, %ecx
    orl %ecx, %ecx
    jz 3f
    addl $(4096-1), %ecx
    andl $~(4096-1), %ecx
    movl %ecx, %ebx
    shrl $16, %ebx
    andl $0xffff, %ecx
    /* alloc mem */
    movl $0x501, %eax
    int $0x31
    jc 2f
    movw %cx, maddr
    movw %bx, maddr + 2
    movw %di, handle
    movw %si, handle + 2
    /* set base */
    movw maddr, %dx
    movw maddr + 2, %cx
    movw sel, %bx
    movl $7, %eax
    int $0x31
    /* set limit */
    movw sel, %bx
    movw %fs:STUBINFO_SELF_SIZE, %dx
    decw %dx
    movw %fs:STUBINFO_SELF_SIZE + 2, %cx
    movl $8, %eax
    int $0x31
    /* seek fd */
    movl %fs:STUBINFO_SELF_FD, %ebx
    movzwl %fs:STUBINFO_SELF_OFFS, %edx
    movzwl %fs:STUBINFO_SELF_OFFS + 2, %ecx
    movl $0x4200, %eax
    int $0x21
    /* read payload */
    pushl %ds
    movw sel, %ds
    movl %fs:STUBINFO_SELF_FD, %ebx
    movl %fs:STUBINFO_SELF_SIZE, %ecx
    movl $0, %edx
    movl $0x3f00, %eax
    int $0x21
    popl %ds
    jc 2f
    /* close file */
    movl %fs:STUBINFO_SELF_FD, %ebx
    movl $0x3e00, %eax
    int $0x21
    /* free desc */
    movw sel, %bx
    movl $1, %eax
    int $0x31
    /* call manager */
    movl %fs:STUBINFO_FLAGS, %eax
    movl maddr, %ebx
    movl %fs:STUBINFO_SELF_SIZE, %ecx
    movl %fs:STUBINFO_MEM_BASE, %edx
    call ctrl
    jc 2f
    /* free mem */
    movl $0x502, %eax
    movw handle, %di
    movw handle + 2, %si
    int $0x31
    jc 2f
11:
    popl %es

1:
    ret

321:
    // no payload found
    movl $err2_str, %edx
    jmp perr

2:
    movl $err_str, %edx
perr:
    movl $0x900, %eax
    int $0x21
    popl %es
    movl $0x4c01, %eax
    int $0x21
    jmp 1b

3:
    /* close file */
    movl %fs:STUBINFO_SELF_FD, %ebx
    movl $0x3e00, %eax
    int $0x21
    /* call manager */
    movl %fs:STUBINFO_FLAGS, %eax
    movl $0, %ebx
    movl $0, %ecx
    movl %fs:STUBINFO_MEM_BASE, %edx
    call ctrl
    jc 2b
    jmp 11b

ctrl:
    pushal
    movl %esp, %edx
    movl __plt_handle, %eax
    movl $0, %ebx
    movb $AUX_CORE, %bl
    movb $DL_API_VER, %bh
    movl $DL_SET_SYMTAB, %ecx
    movl %cs, %esi
    lcalll *__plt_ctrl
    testl %eax, %eax
    jnz 1f
    popal
    clc
    ret
1:
    popal
    stc
    ret

.global plt_done
plt_done:
    // close lib
    movl __plt_handle, %eax
    movl $PLT_CLOSE_CMD, %ebx
    movw shm_block + SHM_HANDLE, %di
    movw shm_block + SHM_HANDLE + 2, %si
    lcalll *__plt_open
    /* free mem */
    movl $0xd01, %eax
    movw shm_block + SHM_HANDLE, %di
    movw shm_block + SHM_HANDLE + 2, %si
    int $0x31
    /* free mem */
    movl $0xd01, %eax
    movw shm_block2 + SHM_HANDLE, %di
    movw shm_block2 + SHM_HANDLE + 2, %si
    int $0x31
    ret

.bss

.global _dosobj_page
_dosobj_page: .space 4096
