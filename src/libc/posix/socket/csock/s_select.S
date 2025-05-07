#include <libc/asmdefs.h>
#define USE_EBX
#define USE_EDX
#define USE_EDI
#define USE_EAX
#define USE_ESI
#define USE_ECX

.text

FUNC(____csock_select)
    ENTER

    movl ARG1, %esi
    movl (%esi), %ebx
    movl ARG2, %esi
    movl (%esi), %ecx
    movl ARG3, %esi
    movl (%esi), %edx
    movl ARG4, %esi
    movl (%esi), %edi
    movl $6, %eax
    lcall *_csock_entry       // Call the entry point for SOCK.VXD
    jc 1f                     // Jump to label 1 if the carry flag is set (error)
    movl ARG1, %esi
    movl %ebx, (%esi)
    movl ARG2, %esi
    movl %ecx, (%esi)
    movl ARG3, %esi
    movl %edx, (%esi)
    movl ARG4, %esi
    movl %edi, (%esi)
    xorl %eax, %eax           // Clear EAX (set it to 0) on success
2:
    LEAVE
1:
    neg %eax
    jmp 2b
