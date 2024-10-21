/* Copyright (C) 1994, 1995 Charles Sandmann (sandmann@clio.rice.edu)
   Exception handling and basis for signal support for DJGPP V2.0
   This software may be freely distributed, no warranty. */

#include <libc/stubs.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#include <unistd.h>
#include <io.h>
#include <libc/farptrgs.h>
#include <dpmi.h>
#include <go32.h>
#include <dos.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <crt0.h>
#include <pc.h>
#include <sys/exceptn.h>
#include <sys/nearptr.h>		/* For DS base/limit info */
#include <libc/internal.h>
#include <libc/djctx.h>
#include <stubinfo.h>

#define err(x) _write(STDERR_FILENO, x, sizeof(x)-1)

//extern unsigned end __asm__ ("end");
static unsigned char old_video_mode = 3;
static int cbrk_vect = 0x1b;		/* May be 0x06 for PC98 */

static void
itox(int v, int len)
{
  static char buf[9]; /* startup default to all NULLs */
  char *bp = buf+7;
  while (len)
  {
    int d = v & 15;
    v >>= 4;
    *bp = d + '0';
    if (d > 9)
      *bp += 'a' - '9' - 1;
    bp--;
    len--;
  }
  _write(STDERR_FILENO, bp+1, 7-(bp-buf));
}

static int
except_to_sig(int excep)
{
  switch(excep)
  {
  case 5:
  case 8:
  case 9:
  case 11:
  case 12:
  case 13:
  case 14:
    return SIGSEGV;
  case 0:
  case 4:
  case 16:
    return SIGFPE;
  case 1:
  case 3:
    return SIGTRAP;
  case 7:
    return SIGNOFP;
  default:
    if(excep == 0x75)		/* HW int to fake exception values hardcoded in exceptn.S */
      return SIGFPE;
    else if(excep == 0x78)
      return SIGTIMR;
    else if(excep == 0x79 || excep == 0x1b)
      return SIGINT;
    else if(excep == 0x7a)
      return SIGQUIT;
    else
      return SIGILL;
  }
}

#if 0
static void
show_call_frame(void)
{
  unsigned *vbp, *vbp_new, *tos;
  unsigned veip;
  int max;

  if (isatty(STDERR_FILENO))
  {
    max =_farpeekb(_dos_ds, 0x484) + 1;	/* number of screen lines */
    if (max < 10 || max > 75)	/* sanity check */
      max = 10;			/* 10 worked for v2.0 and v2.01 */
    else
      max -= 15;		/* 13 lines of preamble + 2 for symify */
  }
  else
    /* This could be MAX_INT.  But the stack can be scrogged, so we
       limit by the maximum that the stack could hold (2 int's per frame).  */
    max = _stklen / (2*sizeof(unsigned));

  tos = (unsigned *)(uintptr_t)__djgpp_selector_limit;
  vbp = (unsigned *)(uintptr_t)__djgpp_exception_state->__ebp;
  err("Call frame traceback EIPs:\r\n  0x");
  itox(__djgpp_exception_state->__eip, 8);
  max--;
  while (((uintptr_t)vbp >= __djgpp_exception_state->__esp) && (vbp >= &end) && (vbp < tos))
  {
    vbp_new = (unsigned *)(uintptr_t)*vbp;
    if (vbp_new == 0)
      break;
    veip = *(vbp + 1);
    err("\r\n  0x");
    itox(veip, 8);
    vbp = vbp_new;
    if (--max <= 0)
      break;
  }
  err("\r\n");
}
#endif

static const char *exception_names[] = {
  "Division by Zero",
  "Debug",
  "NMI",
  "Breakpoint",
  "Overflow",
  "Bounds Check",
  "Invalid Opcode",
  "Coprocessor not available",
  "Double Fault",
  "Coprocessor overrun",
  "Invalid TSS",
  "Segment Not Present",
  "Stack Fault",
  "General Protection Fault",
  "Page fault",
  0,
  "Coprocessor Error",
  "Alignment Check"
};

#define EXCEPTION_COUNT (sizeof(exception_names)/sizeof(exception_names[0]))

static char has_error[] = {0,0,0,0,0,0,0,0 ,1,0,1,1,1,1,1,0 ,0,1 };

static void
dump_selector(const char *name, int sel)
{
  ULONG32 base;
  unsigned limit;
  _write(STDERR_FILENO, name, 2);
  err(": sel="); itox(sel, 4);
  if (sel) {
    if (__dpmi_get_segment_base_address(sel, &base))
    {
      err("  invalid");
    }
    else
    {
      err("  base="); itox(base, 8);
      limit = __dpmi_get_segment_limit(sel);
      err("  limit="); itox(limit, 8);
    }
  }
  err("\r\n");
}

static void
do_faulting_finish_message(int fake_exception)
{
  const char *en;
  unsigned long signum = __djgpp_exception_state->__signum;
  unsigned excpt_stack_addr = (uintptr_t)&djgpp_exception_stack;
  int i;
  const char *prog_name;
#if 0
  /* check video mode for original here and reset (not if PC98) */
  if(ScreenPrimary != 0xa0000 && _farpeekb(_dos_ds, 0x449) != old_video_mode) {
    asm volatile ("pusha;movzbl _old_video_mode,%eax; int $0x10;popa;nop");
  }
#endif
  en = (signum >= EXCEPTION_COUNT) ? 0 :
  exception_names[signum];
  if (signum == 0x75)
    en = "Floating Point exception";
  else if (signum == 0x1b)
    en = "Control-Break Pressed";
  else if (signum == 0x79)
    en = "INTR key Pressed";
  else if (signum == 0x7a)
    en = "QUIT key Pressed";
  if (en == 0)
  {
    if (fake_exception)
      err("Raised");
    else
    {
      err("Exception ");
      itox(signum, 2);
    }
  }
  else
    _write(STDERR_FILENO, en, strlen(en));

  err(" at eip=");
#if 0
  /* For fake exceptions like SIGABRT report where `raise' was called.  */
  if (fake_exception
      && __djgpp_exception_state->__cs == _my_cs()
      && (unsigned)__djgpp_exception_state->__ebp
		>= __djgpp_exception_state->__esp
      && (unsigned *)__djgpp_exception_state->__ebp >= &end
      && __djgpp_exception_state->__ebp
		< (unsigned)__djgpp_selector_limit)
    itox(*((unsigned *)__djgpp_exception_state->__ebp + 1), 8);
  else
#endif
    itox(__djgpp_exception_state->__eip, 8);

  if (signum == 0x79)
  {
    err("\r\n");
    exit(-1); /* note: `exit', not `_exit' as for the rest of signals! */
  }
  if (signum <= EXCEPTION_COUNT && has_error[signum])
  {
    unsigned int errorcode = __djgpp_exception_state->__sigmask & 0xffff;
    if(errorcode)
    {
      err(", error="); itox(errorcode, 4);
    }
  }
  if (except_to_sig(signum) == SIGFPE)
  {
    err(", x87 status="); itox(_status87(), 4);
  }
  err("\r\neax="); itox(__djgpp_exception_state->__eax, 8);
  err(" ebx="); itox(__djgpp_exception_state->__ebx, 8);
  err(" ecx="); itox(__djgpp_exception_state->__ecx, 8);
  err(" edx="); itox(__djgpp_exception_state->__edx, 8);
  err(" esi="); itox(__djgpp_exception_state->__esi, 8);
  err(" edi="); itox(__djgpp_exception_state->__edi, 8);
  err("\r\nebp="); itox(__djgpp_exception_state->__ebp, 8);
  err(" esp="); itox(__djgpp_exception_state->__esp, 8);
  err(" program=");
  prog_name = __dos_argv0 ? __dos_argv0 : "<**UNKNOWN**>";
  for (i=0; prog_name[i]; i++);
  _write(STDERR_FILENO, prog_name, i);
  err("\r\n");
  dump_selector("cs", __djgpp_exception_state->__cs);
  dump_selector("ds", __djgpp_exception_state->__ds);
  dump_selector("es", __djgpp_exception_state->__es);
  dump_selector("fs", __djgpp_exception_state->__fs);
  dump_selector("gs", __djgpp_exception_state->__gs);
  dump_selector("ss", __djgpp_exception_state->__ss);
  err("App stack: ["); itox(__djgpp_stack_top, 8);
  err(".."); itox(__djgpp_stack_limit, 8);
  err("]  Exceptn stack: ["); itox(excpt_stack_addr+8000, 8);
  err(".."); itox(excpt_stack_addr, 8); err("]\r\n");
  err("\r\n");
//  if (__djgpp_exception_state->__cs == _my_cs())
//    show_call_frame();
  _exit(-1);
}

typedef void (*SignalHandler) (int);

static SignalHandler signal_list[SIGMAX];	/* SIG_DFL = 0 */

SignalHandler
signal(int sig, SignalHandler func)
{
  SignalHandler temp;
  if(sig <= 0 || sig > SIGMAX || sig == SIGKILL)
  {
    errno = EINVAL;
    return SIG_ERR;
  }
  temp = signal_list[sig - 1];
  signal_list[sig - 1] = func;
  return temp;
}

static const char signames[] = "ABRTFPE ILL SEGVTERMALRMHUP INT KILLPIPEQUITUSR1USR2NOFPTRAP";

static void print_signal_name(int sig)
{
  err("\r\n");
  err("Exiting due to signal ");
  if (sig >= SIGABRT && sig <= SIGTRAP)
  {
    err("SIG");
    _write(STDERR_FILENO, signames+(sig-SIGABRT)*4, 4);
  }
  else
  {
    err("0x");
    itox(sig, 4);
  }
  err("\r\n");
}

void __djgpp_traceback_exit(int);

void __djgpp_traceback_exit(int sig)
{
#if 0
  jmp_buf fake_exception;

  if (sig >= SIGABRT && sig <= SIGTRAP)
  {
    if (!*____djgpp_exception_state_ptr)
    {
      /* This is a software signal, like SIGABRT or SIGKILL.
	 Fill the exception structure, so we get the traceback.  */
      *____djgpp_exception_state_ptr = PTR_DATA(&fake_exception);
      if (setjmp(__djgpp_exception_state))
      {
	err("Bad longjmp to __djgpp_exception_state--aborting\r\n");
	do_faulting_finish_message(0); /* does not return */
      }
      else
	/* Fake the exception number.  7Ah is the last one hardwired
	   inside exceptn.S, for SIGQUIT.  */
	__djgpp_exception_state->__signum = 0x7a + 1 + sig - SIGABRT;
    }
  }
#endif
  print_signal_name(sig);
//  if(__djgpp_exception_state_ptr)
    /* This exits, does not return.  */
//    do_faulting_finish_message(__djgpp_exception_state_ptr == &fake_exception);
  _exit(-1);
}

int
raise(int sig)
{
  SignalHandler temp;

  if(sig <= 0)
    return -1;
  if(sig > SIGMAX)
    return -1;
  temp = signal_list[sig - 1];
  if(temp == (SignalHandler)SIG_IGN
     || (sig == SIGQUIT && temp == (SignalHandler)SIG_DFL))
    return 0;			/* Ignore it */
  if(temp == (SignalHandler)SIG_DFL)
    __djgpp_traceback_exit(sig); /* this does not return */
  else if((uintptr_t)temp < 4096)
  {
    err("Bad signal handler, ");
    __djgpp_traceback_exit(sig); /* does not return */
  }
  else
    temp(sig);
  return 0;
}

/* This routine must call exit() or jump changing stacks.  This routine is
   the basis for traceback generation, core creation, signal handling. */
void
__djgpp_exception_processor(void)
{
  int sig;

  sig = except_to_sig(__djgpp_exception_state->__signum);
  raise(sig);
#if 0
  if(__djgpp_exception_state->__signum >= EXCEPTION_COUNT) /* Not exception so continue OK */
    longjmp(__djgpp_exception_state, __djgpp_exception_state->__eax);
  /* User handler did not exit or longjmp, we must exit */
#endif
  err("Cannot continue from exception, ");
  print_signal_name(sig);
  do_faulting_finish_message(0);
}

struct exc_info {
  __dpmi_paddr except_ori[EXCEPTION_COUNT];
  __dpmi_paddr kbd_ori;
  __dpmi_paddr npx_ori;
  __dpmi_paddr timer_ori;
  __dpmi_raddr cbrk_ori;
  __dpmi_raddr cbrk_rmcb;
  char cbrk_hooked;
  ULONG32 cbrk_regs;
};

static struct exc_info *xinfo;
DJ64_DEFINE_SWAPPABLE_CONTEXT(exc_info, xinfo)
#define except_ori xinfo->except_ori
#define kbd_ori xinfo->kbd_ori
#define npx_ori xinfo->npx_ori
#define timer_ori xinfo->timer_ori
#define cbrk_ori xinfo->cbrk_ori
#define cbrk_rmcb xinfo->cbrk_rmcb
#define cbrk_hooked xinfo->cbrk_hooked
#define cbrk_regs xinfo->cbrk_regs

/* Routine toggles ALL the exceptions.  Used around system calls, at exit. */

void
__djgpp_exception_toggle(void)
{
  __dpmi_paddr except;
  size_t i;

  for(i=0; i < EXCEPTION_COUNT; i++)
  {
    __dpmi_get_processor_exception_handler_vector(i, &except);
    if(i != 2 || (_crt0_startup_flags & _CRT0_FLAG_NMI_SIGNAL))
      __dpmi_set_processor_exception_handler_vector(i, &except_ori[i]);
    except_ori[i] = except;
  }
  __dpmi_get_protected_mode_interrupt_vector(0x75, &except);
  __dpmi_set_protected_mode_interrupt_vector(0x75, &npx_ori);
  npx_ori = except;
  __dpmi_get_protected_mode_interrupt_vector(9, &except);
  __dpmi_set_protected_mode_interrupt_vector(9, &kbd_ori);
  kbd_ori = except;
  if(cbrk_hooked) {
    __dpmi_set_real_mode_interrupt_vector(cbrk_vect, &cbrk_ori);
    __dpmi_free_real_mode_callback(&cbrk_rmcb);
    cbrk_hooked = 0;
  } else {
    __dpmi_get_real_mode_interrupt_vector(cbrk_vect, &cbrk_ori);
    __dpmi_allocate_real_mode_callback(__djgpp_cbrk_hdlr, cbrk_regs, &cbrk_rmcb);
    __dpmi_set_real_mode_interrupt_vector(cbrk_vect, &cbrk_rmcb);
    cbrk_hooked = 1;
  }
  /* If the timer interrupt is hooked, toggle it as well.  This is so
     programs which use SIGALRM or itimer, and don't unhook the timer before
     they exit, won't leave the system with timer pointing into the void.  */
  if (__djgpp_old_timer.selector != 0 && __djgpp_old_timer.offset32 != 0)
  {
    if (timer_ori.selector == __djgpp_old_timer.selector
	&& timer_ori.offset32 == __djgpp_old_timer.offset32)
    {
      __dpmi_get_protected_mode_interrupt_vector(8, &timer_ori);
      __dpmi_set_protected_mode_interrupt_vector(8, &__djgpp_old_timer);
    }
    else
    {
      __dpmi_set_protected_mode_interrupt_vector(8, &timer_ori);
      timer_ori = __djgpp_old_timer;
    }
  }
}

#define RSHIFT 1
#define LSHIFT 2
#define CTRL   4
#define ALT    8
#define SHIFT  (RSHIFT | LSHIFT)

#define DEFAULT_SIGINT  0x042e /* Ctrl-C: scan code 2Eh, kb status 04h */
#define DEFAULT_SIGQUIT 0x042b /* Ctrl-\: scan code 2Bh, kb status 04h */
#define DEFAULT_SIGINT_98  0x042b /* Ctrl-C: scan code 2Bh, kb status 04h */
#define DEFAULT_SIGQUIT_98 0x040d /* Ctrl-\: scan code 0Dh, kb status 04h */

/* Make it so the key NEW_KEY will generate the signal SIG.
   NEW_KEY must include the keyboard status byte in bits 8-15 and the
   scan code in bits 0-7.  */
static int
set_signal_key(int sig, int new_key)
{
  int old_key;
  unsigned short *mask;
  unsigned short *key;
  unsigned short kb_status;

  if (sig == SIGINT)
  {
    mask = &__djgpp_sigint_mask;
    key  = &__djgpp_sigint_key;
  }
  else if (sig == SIGQUIT)
  {
    mask = &__djgpp_sigquit_mask;
    key  = &__djgpp_sigquit_key;
  }
  else
    return -1;

  old_key = *key;

  *key = new_key & 0xffff;
  kb_status = *key >> 8;

  *mask = 0x000f;	/* Alt, Ctrl and Shift bits only */
  /* Mask off the RShift bit unless they explicitly asked for it.
     Our keyboard handler pretends that LShift is pressed when they
     press RShift.  */
  if ((kb_status & RSHIFT) == 0)
    *mask &= ~RSHIFT;
  /* Mask off the LShift bit if any of the Ctrl or Alt are set,
     since Shift doesn't matter when Ctrl and/or Alt are pressed.  */
  if (kb_status & (CTRL | ALT))
    *mask &= ~LSHIFT;

  return old_key;
}

int
__djgpp_set_sigint_key(int new_key)
{
  return set_signal_key(SIGINT, new_key);
}

int
__djgpp_set_sigquit_key(int new_key)
{
  return set_signal_key(SIGQUIT, new_key);
}


void
__djgpp_signal_setup(void)
{
  int i;

  for (i = 0; i < SIGMAX; i++)
    signal_list[i] = (SignalHandler)SIG_DFL;
}

void
__djgpp_exception_setup(void)
{
  __dpmi_paddr except;
  __dpmi_meminfo lockmem, regmem;
  size_t i;

  __excep_ds_alias = __djgpp_ds_alias;
  if (ScreenPrimary != 0xa0000)
    {
      __djgpp_set_sigint_key(DEFAULT_SIGINT);
      __djgpp_set_sigquit_key(DEFAULT_SIGQUIT);
    }
  else
    {				/* for PC98 */
      __djgpp_set_sigint_key(DEFAULT_SIGINT_98);
      __djgpp_set_sigquit_key(DEFAULT_SIGQUIT_98);
    }

  __djgpp_signal_setup();

  /* app_DS only used when converting HW interrupts to exceptions */
  __djgpp_app_DS = _my_ds();
  __djgpp_our_DS = _my_ds();
  __djgpp_dos_sel = _dos_ds;

  regmem.size = sizeof(__dpmi_regs);
  __dpmi_allocate_memory(&regmem);
  cbrk_regs = regmem.address - __djgpp_base_address;

  /* lock addresses which may see HW interrupts */
  lockmem.address = __djgpp_base_address + (uintptr_t) &__djgpp_hw_lock_start;
  lockmem.size = ((uintptr_t) &__djgpp_hw_lock_end
		  - (uintptr_t) &__djgpp_hw_lock_start);
  __dpmi_lock_linear_region(&lockmem);

  except.selector = _my_cs();
  except.offset32 = (uintptr_t) __djgpp_exception_table;
  for(i=0; i < EXCEPTION_COUNT; i++)
  {
    except_ori[i] = except;	/* New value to set */
    except.offset32 += 4;	/* This is the size of push n, jmp */
  }
  kbd_ori.selector = npx_ori.selector = except.selector;
  npx_ori.offset32 = __djgpp_npx_hdlr;
  if(ScreenPrimary != 0xa0000)
    kbd_ori.offset32 = __djgpp_kbd_hdlr;
  else
  {
    kbd_ori.offset32 = __djgpp_kbd_hdlr_pc98;
    cbrk_vect = 0x06;
    except.offset32 = __djgpp_iret;		/* TDPMI98 bug */
    __dpmi_set_protected_mode_interrupt_vector(0x23, &except);
  }
  except.offset32 = __djgpp_i24;
  __dpmi_set_protected_mode_interrupt_vector(0x24, &except);

  __dpmi_get_protected_mode_interrupt_vector(9, &__djgpp_old_kbd);
  __dpmi_get_protected_mode_interrupt_vector(8, &timer_ori);
  __djgpp_exception_toggle();	/* Set new values & save old values */

  /* get original video mode and save */
  old_video_mode = _farpeekb(_dos_ds, 0x449);
}

int
__djgpp_set_ctrl_c(int enable_sigs)
{
  int oldenable = !(__djgpp_hwint_flags & 1);
  if (enable_sigs >= 0)
  {
    if (enable_sigs)
      __djgpp_hwint_flags &= ~1;
    else
      __djgpp_hwint_flags |= 1;
  }
  return oldenable;
}

/* A work-around for a bug in W2K's NTVDM, suggested by
   The Owl <theowl@freemail.c3.hu>.

   When a DPMI program exits, NTVDM frees up all memory blocks allocated
   by the program and then it resets its notion of the current program's
   PSP selector to 0.  If that DPMI program was spawned by another DPMI
   program, and the parent program exits while the PSP selector recorded
   by NTVDM is still 0, NTVDM will free up all memory blocks allocated
   by DOSX, the NT DOS extender.  That memory includes the locked PM
   stack used for processing hardware interrupts reflected into protected
   mode.  Thereafter, any hardware interrupt (a timer tick, a keyboard
   keypress, anything) will crash NTVDM because NTVDM will try to use a
   stack which was freed.

   The bug which causes this is that NTVDM resets the PSP to 0,
   instead of setting it to the PSP of the parent program.  (Ironically,
   DOSX does pass the parent PSP selector to NTVDM when it notifies it
   about a DPMI program exit, but NTVDM does not use this information.)

   To work around that, we ask NTVDM to record a valid PSP before we
   exit.  We do that by invoking a PM Int 21h, function 50h, which sets
   the PM selector for the PSP (which triggers NTVDM to record the
   correct internal value).  We do that just before exiting, to make
   sure that even an application which crashes (e.g., due to SIGSEGV
   or Ctrl-BREAK) immediately after its child returns will always leave
   its valid PSP recorded by NTVDM before it exits.

   (To play it safe in the face of non-DJGPP DPMI programs and old
   DJGPP programs, we also restore the PSP in dosexec.c, which see.)

   Note that we invoke here a PM Int 21, which sets the PM selector of
   our PSP.  This is _not_ a call to __dpmi_int ! */
#ifndef DJ64
static void
__maybe_fix_w2k_ntvdm_bug(void)
{
  if (_os_trueversion == 0x532) /* Windows NT, 2000 or XP? */
  {
   if(_stubinfo->size < STUBINFO_END)	/* V2load'ed image, stubinfo PSP bad */

    /* Protected mode call to SetPSP - uses BX from GetPSP (0x51) */
    asm volatile("movb $0x51, %%ah                        \n\
                  int  $0x21                              \n\
                  movb $0x50, %%ah                        \n\
                  int  $0x21                              "
                  : : : "ax", "bx" );             /* output, input, regs */
   else

    /* Protected mode call to SetPSP - may destroy RM PSP if not extended */
    asm volatile("movw %0, %%bx                           \n\
                  movb $0x50, %%ah                        \n\
                  int  $0x21                              "
                  :                               /* output */
                  : "g" (_stubinfo->psp_selector) /* input */
                  : "ax", "bx" );                 /* regs */
  }
}
#endif

void
_exit(int status)
{
#ifndef DJ64
  __maybe_fix_w2k_ntvdm_bug();
#endif
  /* If we are exiting due to an FP exception, the next program run in the
     same DOS box on Windows crashes during startup.  Clearing the 80x87
     seems to prevent this, at least in some cases.  We only do that if a
     coprocessor is actually present.  */
  if (_8087)
    _clear87();
  /* We need to restore hardware interrupt handlers even if somebody calls
     `_exit' directly, or else we crash the machine in nested programs.
     We only toggle the handlers if the original keyboard handler is intact
     (otherwise, they might have already toggled them).  */
  if (__djgpp_old_kbd.offset32 == kbd_ori.offset32
      && __djgpp_old_kbd.selector == kbd_ori.selector)
    __djgpp_exception_toggle ();
  __exit (status);
}

void __attribute__((alias("_exit")))
_Exit(int status);

#ifdef TEST

#include <string.h>

int
main(void)
{
  volatile int count = 0;	/* don't let gcc optimize it away */

  __djgpp_set_sigint_key (0x0522); /* Ctrl-RShift-G */
  signal (SIGQUIT, __djgpp_traceback_exit);

  while (1)
  {
    char buf[30];

    count++;
    if (count % 100000 == 0)
    {
      sprintf (buf, "counted to %d\r\n", count);
      _write(STDERR_FILENO, buf, strlen (buf));
    }
    if (count >= 1000000000L)
      count = 0;
  }

  return 0;
}

#endif
