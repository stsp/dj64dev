#include <libc/asmdefs.h>
#define USE_EBX
#define USE_EDX
#define USE_EDI
#define USE_EAX
#define USE_ESI
#define USE_ECX

.text

FUNC(____csock_sendto)
    ENTER

    movl ARG1, %edi
    movl ARG2, %ebx
    movl ARG3, %edx
    movl ARG4, %esi
    movl ARG5, %ecx
    movl $4, %eax
    lcall *_csock_entry       // Call the entry point for SOCK.VXD
    jc 1f                     // Jump to label 1 if the carry flag is set (error)
    movl ARG6, %ebx
    movl %ecx, (%ebx)
    xorl %eax, %eax           // Clear EAX (set it to 0) on success
2:
    LEAVE
1:
    neg %eax
    jmp 2b
