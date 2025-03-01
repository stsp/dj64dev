.macro handler_prolog len
    push %ds
    mov %cs:__rmcb_ds, %eax
    mov %eax, %ds
.ifndef _DJ64
    /* djgpp needs emergency stack because of a bug in __dpmi_int() */
    mov %ss, %esi
    mov %esp, %edi
    pushl __rmcb_ds
    lea _cstack+\len, %edx
    push %edx
    lss (%esp), %esp
    push %esi
    push %edi
.endif
.endm

.macro restore_stack
.ifndef _DJ64
    lss (%esp), %esp
.endif
    pop %ds
.endm
