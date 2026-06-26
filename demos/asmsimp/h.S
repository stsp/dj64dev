#include <libc/asmdefs.h>
#ifndef DJ64
#define RET_I ret
#endif

.text
    .global _hello_asm
_hello_asm:
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
    movl $msg, %edx
    int $0x21
    jc 1f
    xor %eax, %eax
2:
    RET_I
1:
    mov $-1, %eax
    jmp 2b

.data
msg: .asciz "Hello from ASM\n$"
dos_nm: .asciz "MS-DOS"
