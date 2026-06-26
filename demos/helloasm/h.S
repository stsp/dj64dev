#include <libc/asmdefs.h>
#ifndef DJ64
#define RET_I ret
#endif

.text
    .global _hello_asm
_hello_asm:
    call _copy_msg

    /* check for extender, used for file I/O and printing */
    movl $0x168a, %eax
    movl $dos_nm, %esi
    clc
    push %es
    int $0x2f
    pop %es
    jc 1f
    orb %al, %al
    jnz 1f  /* unsupported */

    movl $0x900, %eax
    movl $_msg, %edx
    int $0x21
    jc 1f
    xor %eax, %eax
2:
    RET_I
1:
    mov $-1, %eax
    jmp 2b

.bss
    .global _msg
_msg: .space 128

.data
dos_nm: .asciz "MS-DOS"
