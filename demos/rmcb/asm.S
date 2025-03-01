#include <libc/asmdefs.h>
#include "asm.h"

.bss
.global __rmcb_ds
__rmcb_ds: .long 0
.ifndef _DJ64
/* We need emergency stack because __dpmi_int() alters SS in djgpp.
 * That was eventually fixed in dj64, hence the ifdef. */
.global _cstack
_cstack: .fill SIGSTK_LEN
.endif
