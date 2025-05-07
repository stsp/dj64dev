#include <libc/asmdefs.h>
#define USE_EBX
#define USE_EDX
#define USE_EDI
#define USE_EAX
#define USE_ESI
#define USE_ECX

.text

FUNC(____csock_recvfrom)
    ENTER

    movl ARG1, %edi
    movl ARG2, %esi
    movl ARG3, %ecx
    movl $5, %eax
    lcall *_csock_entry       // Call the entry point for SOCK.VXD
    jc 1f                     // Jump to label 1 if the carry flag is set (error)
    movl ARG4, %edi
    movl %ecx, (%edi)
    movl ARG5, %edi
    movl %ebx, (%edi)
    movl ARG6, %edi
    movl %edx, (%edi)
    xorl %eax, %eax           // Clear EAX (set it to 0) on success
2:
    LEAVE
1:
    neg %eax
    jmp 2b
