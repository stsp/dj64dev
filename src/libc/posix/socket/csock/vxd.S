#include <libc/asmdefs.h>
#define USE_EBX
#define USE_EAX
#define USE_EDI
#define USE_ESI

.data

.global _csock_entry
_csock_entry:
csock_entry_ip: .long 0
csock_entry_cs: .long 0

.text

FUNC(_VxdGetEntry)
    ENTER

    movl ARG1, %ebx
    movl $0x1684, %eax
    xorl %edi, %edi
    push %es
    movl %edi, %es
    int $0x2f
    movl %es, %ebx
    pop %es
    jc 1f
    testl %ebx, %ebx
    jz 1f
    movl %ebx, csock_entry_cs
    movl %edi, csock_entry_ip
    xorl %eax, %eax           // Clear EAX (set it to 0) on success
2:
    LEAVE
1:
    movl $-1, %eax
    jmp 2b
