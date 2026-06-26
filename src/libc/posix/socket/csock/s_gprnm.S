#define USE_EBX
#define USE_EDI
#define USE_EAX
#define USE_ECX
#include <libc/asmdefs.h>

.text

FUNC(____csock_getpeername)
    ENTER

    movl ARG1, %edi
    movl $14, %eax
    lcall *_csock_entry       // Call the entry point for SOCK.VXD
    jc 1f                     // Jump to label 1 if the carry flag is set (error)
    movl ARG2, %edi
    movl %ecx, (%edi)
    movl ARG3, %edi
    movw %bx, (%edi)
    xorl %eax, %eax           // Clear EAX (set it to 0) on success
2:
    LEAVE
1:
    neg %eax
    jmp 2b
