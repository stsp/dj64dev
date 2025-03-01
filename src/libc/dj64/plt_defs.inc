.macro call_plt
    movl __plt_handle, %eax
    movl $0, %esi  # reserved
    lcalll *__plt_call
.endm
