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

#include <libc/djthunks.h>

#ifndef _f
#define _h #
#define _f(x) x
#define DEFINE(x, y) _f(_h)define x y
#endif

#define __ASYM(x) __##x()
#define __ASYM_L(x) (*__##x())

#define __S(x) #x
#define _S(x) __S(x)

#ifndef IN_ASMOBJ

#ifdef _IN_DJ64
#define ASM(t, x) t *__##x(void);/*
*/DEFINE(x, (*__##x()))/*
*/
#else
#define ASM(t, x) t *__##x(void)
#endif

#define ASMh(t, x) \
t *___##x(int handle) \
{ \
  return (t *)djaddr2ptr2(djthunk_get_h(handle, _S(_##x)), sizeof(t)); \
}

#ifdef _IN_DJ64
#define ASM_N(t, x) t *__##x(void);/*
*/DEFINE(x, (*__##x()))/*
*/
#else
#define ASM_N(t, x) t *__##x(void)
#endif

#ifdef _IN_DJ64
#define ASM_P(t, x) t *__##x(void);/*
*/DEFINE(x, __##x())/*
*/
#else
#define ASM_P(t, x) t *__##x(void)
#endif

#ifdef _IN_DJ64
#define ASM_ARR(t, x, l) t *__##x(void);/*
*/DEFINE(x, __##x())/*
*/
#else
#define ASM_ARR(t, x, l) t *__##x(void)
#endif

#ifdef _IN_DJ64
#define ASM_F(x) unsigned __##x(void);/*
*/DEFINE(x, __##x())/*
*/
#else
#define ASM_F(x) unsigned __##x(void)
#endif

#define EXTERN extern

#else

#if IN_ASMOBJ == 1
#define ASM_N(t, x) t _##x
#endif

#if IN_ASMOBJ == 2
#define ASM_N(t, x) t *__##x(void)
#define ASM_Ni(t, x) \
t *__##x(void) \
{ \
  return (t *)djaddr2ptr2(djthunk_get(_S(_##x)), sizeof(t)); \
}
#endif

#if IN_ASMOBJ == 3

#undef ASM
#define ASM(t, x) \
t *__##x(void) \
{ \
  return (t *)djaddr2ptr2(djthunk_get(_S(_##x)), sizeof(t)); \
}

unsigned *____djgpp_base_address(void);

#undef ASM_F
#define ASM_F(x) \
unsigned __##x(void) \
{ \
  return (djthunk_get(_S(_##x)) - *____djgpp_base_address()); \
}

#undef ASM_N
#define ASM_N(t, x) t *__##x(void)

#define _DP(l, s) \
  djaddr2ptr2((*____djgpp_base_address()) + (l), s)
#undef ASM_P
#define ASM_P(t, x) \
t *__##x(void) \
{ \
  return (t *)_DP(*(unsigned *)djaddr2ptr(djthunk_get(_S(_##x))), sizeof(t)); \
}

#undef ASM_ARR
#define ASM_ARR(t, x, l) \
t *__##x(void) \
{ \
  return (t *)djaddr2ptr2(djthunk_get(_S(_##x)), sizeof(t) * (l)); \
}

#endif

#undef EXTERN
#define EXTERN

#endif

#define __out
#define _V_FW(n)
#define _V_BW(n)
