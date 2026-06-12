/*
 *  dj64 - 64bit djgpp-compatible tool-chain
 *  Copyright (C) 2021-2024  @stsp
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libc/asmdefs.h>

.text

.macro asmcfunc_n nm,num
    .global _\nm
    _\nm:
    call \nm
    ret
.endm

#include "plt.inc"

.macro asmsym nm
    .long _\nm
.endm

#define __ASM(t, n) asmsym n
#define __ASM_FAR(t, n) asmsym n
#define __ASM_NEAR(t, n) asmsym n
#define __ASM_ARR(t, n, l) asmsym n
#define __ASM_ARRI(t, n) asmsym n
#define __ASM_ARRI_F(t, n) asmsym n
#define __ASM_FUNC(n) asmsym n
#define SEMIC
__asm_symtab_start:
#include "glob_asm.h"
__asm_symtab_end:
