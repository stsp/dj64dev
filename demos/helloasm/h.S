#include <libc/asmdefs.h>
#ifndef DJ64
#define RET_I ret
#endif

.text
    .global _hello_asm
_hello_asm:
    call _copy_msg
    movl $0x900, %eax
    movl $_msg, %edx
    int $0x21
    RET_I

.bss
    .global _msg
_msg: .space 128
