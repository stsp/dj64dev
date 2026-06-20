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

#include <string.h>
#include <libc/asmobj.h>
#include <dj64/thunks_p.h>
#include <dj64/tg_defs.h>

#ifdef DJ32
#pragma push_macro("ASMFUNC")
#undef ASMFUNC
#define ASMFUNC extern __attribute__((cdecl))
#define _FUNC(x) _##x
#define _CFUNC(x) x
#include "thunk_p1.h"
#pragma pop_macro("ASMFUNC")
#endif
#ifdef _IN_DJ64
#define __CALL(n, s, l, f) dj64_asm_call(n, s, l, f)
#define __CALL_v_nr(n, s, l, f) dj64_asm_noret(n, s, l, f)
#else
#define __CALL(n, s, l, f) dj64_asm_call_u(_handle, _libid, n, s, l, f)
#define __CALL_v_nr(n, s, l, f) dj64_asm_noret_u(_handle, _libid, n, s, l, f)
#endif
#define __CSTK(l) dj64_clean_stk(l)

#define __CNV_PTR(t, d, f, l, t0) t d  = dj64_obj_init(f, l)
#define __CNV_OPTR(t, d, f, l, t0) t d  = dj64_obj_oinit(f, l)
#define __CNV_CPTR(t, d, f, l, t0) t d  = dj64_obj_init(f, l)
#define __CNV_PTR_CCHAR(t, d, f, l, t0) t d = dj64_obj_init(f, strlen(f) + 1)
#define __CNV_CHAR_ARR(t, d, f, l, t0) t d = dj64_obj_init(f, l)
#define __CNV_CHAR_OARR(t, d, f, l, t0) t d = dj64_obj_oinit(f, l)
#define __CNV_PTR_PVOID(t, d, f, l, t0) t d = PTR_DATA(f)
#define __CNV_PTR_CPVOID(t, d, f, l, t0) t d = PTR_DATA(f)
#define __CNV_PTR_VOID(t, d, f, l, t0) t d = dj64_obj_init(f, l)
#define __CNV_PTR_CVOID(t, d, f, l, t0) t d = dj64_obj_init(f, l)
#define __CNV_PTR_OVOID(t, d, f, l, t0) t d = dj64_obj_oinit(f, l)
#define __CNV_SIMPLE(t, d, f, l, t0) t d = (f)

#define _CNV(c, t, at, l, n) c(at, _a##n, a##n, l, t)
#define _L_REF(nl, m) (a##nl * (m))
#define _L_IMM(n, l) (sizeof(a##n[0]) * (l))
#define _L_SZ(n) sizeof(*a##n)

#define U__CNV_PTR(f, d, l) dj64_obj_done(f, d, l)
#define U__CNV_OPTR(f, d, l) dj64_obj_done(f, d, l)
#define U__CNV_CPTR(f, d, l) dj64_rm_dosobj(f, d)
#define U__CNV_PTR_CCHAR(f, d, l) dj64_rm_dosobj(f, d)
#define U__CNV_PTR_CHAR(f, d, l) dj64_obj_done(f, d, strlen(DATA_PTR(d)) + 1)
#define U__CNV_CHAR_ARR(f, d, l) dj64_obj_done(f, d, l)
#define U__CNV_CHAR_OARR(f, d, l) dj64_obj_done(f, d, l)
#define U__CNV_PTR_PVOID(f, d, l)
#define U__CNV_PTR_CPVOID(f, d, l)
#define U__CNV_PTR_VOID(f, d, l) dj64_obj_done(f, d, l)
#define U__CNV_PTR_CVOID(f, d, l) dj64_rm_dosobj(f, d)
#define U__CNV_PTR_OVOID(f, d, l) dj64_obj_done(f, d, l)
#define U__CNV_SIMPLE(f, d, l)

#define _UCNV(c, l, n) U##c(a##n, _a##n, l)

#ifndef _IN_DJ64
static int _handle;
static int _libid;
#define STATIC static
#else
#define STATIC
#endif

#ifndef DJ32
#include "thunk_asms.h"
#else
#include "thunk_p32.h"
#endif

static const struct athunk _asm_pthunks[] = {
#define ASMCSYM(s, n) [n] = { _S(_##s) },
#include "plt_asmc.h"
};
#ifndef _IN_DJ64
static uint32_t asm_tab[_countof(_asm_pthunks)];
#else
#define asm_tab NULL
#endif

STATIC struct athunks pthunks = {
    _asm_pthunks,
    _countof(_asm_pthunks),
    asm_tab,
};

#ifndef _IN_DJ64
__attribute__((constructor))
static void init(void)
{
    register_pthunks(&pthunks, &_handle, &_libid, DJ64_DYNAPI_VER);
}
#endif
