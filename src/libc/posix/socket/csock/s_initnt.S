#include <libc/asmdefs.h>
#define USE_EBX
#define USE_EDI
#define USE_EAX

.text

FUNC(____csock_get_version)
    ENTER

    movl $0xFFFF, %edi        // Pass a bogus socket descriptor (EDI = 0xFFFF)
    movl $0, %eax             // Set EAX to 0 for the version call
    lcall *_csock_entry       // Call the entry point for SOCK.VXD
    jc 1f                     // Jump to label 1 if the carry flag is set (error)
    movl ARG1, %ebx
    movl %eax, (%ebx)
    xorl %eax, %eax           // Clear EAX (set it to 0) on success
2:
    LEAVE
1:
    movl $-1, %eax
    jmp 2b
