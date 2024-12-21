/* Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_setjmp_h_
#define __dj_include_setjmp_h_

#ifdef __cplusplus
namespace std {
  extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#if defined(__clang__) && defined(__aarch64__) && defined(__ANDROID__)
#define CUSTOM_SETJMP 1
#else
#define CUSTOM_SETJMP 0
#endif

#if CUSTOM_SETJMP
typedef unsigned long long int minimal_jmp_buf[14];
#else
typedef void *minimal_jmp_buf[5];
#endif

typedef struct __jmp_buf {
  minimal_jmp_buf buf;
  volatile int rc;
} jmp_buf[1];

#if CUSTOM_SETJMP
__attribute__((returns_twice,always_inline))
static inline int do_setjmp(minimal_jmp_buf _env)
{
    long int ret;
    asm volatile(
                 // x9 is the first of the regular temporary registers
                 // per the above-mentioned Procedule Call Standard document.
                 // Use it as temporary to hold the value of SP, since str does
                 // not accept SP as operand.
      "  mov x9, sp                     \n"
      "  adr x10, _out                  \n"
      // Store the general-purpose registers that we need to save
      // per the above discussion.
      // Note that x30 is the link register.
      "  stp x19, x20, [%[_env], 0]     \n"
      "  stp x21, x22, [%[_env], 0x10]  \n"
      "  stp x23, x24, [%[_env], 0x20]  \n"
      "  stp x25, x26, [%[_env], 0x30]  \n"
      "  stp x27, x28, [%[_env], 0x40]  \n"
      "  stp x29, x30, [%[_env], 0x50]  \n"
      // Store the value of SP,PC.
      "  stp x9, x10,  [%[_env], 0x60]  \n"
      "  mov x0, 0                      \n"
      "_out:                            \n"
      "  mov %x[ret], x0                \n"
      :
      // Outputs
      [ret]"=r"(ret)
      :
      // Inputs
      [_env]"r"(_env)
      :
      // Clobbers.
      // We have used x9 as a temporary
      "x0","x1","x2","x3","x4","x5","x6","x7","x8","x9","x10",
      "x11","x12","x13","x14","x15","x16","x17","x18",
      // We have written to memory locations
      "memory");

          // Direct invokation of setjmp always returns 0.
          // The pseudo returning of the value 1 as a return from longjmp
          // is implemented in longjmp.
   return ret;
}

__attribute__((noreturn))
static inline void do_longjmp(minimal_jmp_buf _env)
{
   asm volatile(
                 // Loads to match the stores in the above setjmp implementation.
      "  ldp x19, x20, [%[_env], 0]     \n"
      "  ldp x21, x22, [%[_env], 0x10]  \n"
      "  ldp x23, x24, [%[_env], 0x20]  \n"
      "  ldp x25, x26, [%[_env], 0x30]  \n"
      "  ldp x27, x28, [%[_env], 0x40]  \n"
      "  ldp x29, x30, [%[_env], 0x50]  \n"
      "  ldp x9, x10,  [%[_env], 0x60]  \n"
      "  mov sp, x9                     \n"
      // return as setjmp for the second time, returning 1.
      // Since we have loaded the link register x30 from the saved buffer
      // that was set by the above setjmp implementation, the 'ret' instruction
      // here is going to return to where setjmp was called.
      // Per the setjmp/longjmp contract, that pseudo second returning
      // of setjmp should return the value 1.
      // x0 is holds the integer return value.
      "  mov x0, 1                      \n"
      "  br x10                         \n"
      :
      // No outputs
      :
      // Inputs
      [_env]"r"(_env)
      :
      // Clobbers.
      // We have used x9 as a temporary
      "x0","x9","x10");

          // This statement is unreachable because the above asm statement
          // unconditionally does a 'ret' instruction. The purpose of this
          // statement is to silence clang warnings about this function returning
          // while having the 'noreturn' attribute.
   __builtin_unreachable();
}
#endif

__attribute__((noreturn))
static inline void longjmp(jmp_buf env, int val)
{
  env[0].rc = val;
#if CUSTOM_SETJMP
  do_longjmp(env[0].buf);
#else
  __builtin_longjmp(env[0].buf, 1);
#endif
}

#if CUSTOM_SETJMP
#define setjmp(env) ( \
{ \
  int _rc = do_setjmp(env[0].buf); \
  (_rc ? env[0].rc : 0); \
})
#else
#define setjmp(env) ( \
{ \
  int _rc = __builtin_setjmp(env[0].buf); \
  (_rc ? env[0].rc : 0); \
})
#endif

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#ifndef __STRICT_ANSI__
#if 0
typedef jmp_buf sigjmp_buf;

int	sigsetjmp(sigjmp_buf env, int savemask);
void	siglongjmp(sigjmp_buf env, int val) __attribute__((__noreturn__));
#endif
#ifndef _POSIX_SOURCE

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
  }
}
#endif

#endif /* !__dj_include_setjmp_h_ */


#if defined(__cplusplus) && !defined(__dj_ENFORCE_ANSI_FREESTANDING)

using std::setjmp;

#ifndef __dj_via_cplusplus_header_

using std::longjmp;
using std::jmp_buf;

#ifndef __STRICT_ANSI__

using std::sigjmp_buf;
using std::sigsetjmp;
using std::siglongjmp;

#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_via_cplusplus_header_ */
#endif /* __cplusplus && !__dj_ENFORCE_ANSI_FREESTANDING */
