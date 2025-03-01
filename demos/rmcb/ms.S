#include <libc/asmdefs.h>
#include "asm.h"
.include "asm.inc"

.text
.global _rmcb_mouse_handler
_rmcb_mouse_handler:
    pusha
    handler_prolog SIGSTK_LEN
    call _do_mouse
    restore_stack
    popa
    iretl
