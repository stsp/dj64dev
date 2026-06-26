#ifndef ASM_H
#define ASM_H

#ifndef __ASSEMBLER__
#ifdef DJ64
#include <dj64/asm_inc.h>
#else
#include "asm_inc.h"
#endif

#define ASMCFUNC
#define ASMFUNC

void ASMCFUNC copy_msg(void);
int ASMFUNC hello_asm(void);

#endif

#endif
