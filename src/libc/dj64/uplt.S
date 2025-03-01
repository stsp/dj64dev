#include <libc/asmdefs.h>
.include "plt_defs.inc"

.bss

AUX_USER: .long 0
__plt_call: .quad 0
__plt_handle: .long 0

.text

.global _start
_start:
    movl 4(%esp), %eax
    movl %eax, __plt_call
    movl 8(%esp), %eax
    movl %eax, __plt_call + 4
    movl 12(%esp), %eax
    movl %eax, __plt_handle
    movl 16(%esp), %eax
    movl %eax, AUX_USER
    ret

.global dj64_plt_call
dj64_plt_call:
    movl AUX_USER, %ebx
    call_plt
    ret
