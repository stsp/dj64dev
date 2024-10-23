/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 Charles Sandmann (sandmann@clio.rice.edu)
   setitimer implmentation - used for profiling and alarm
   BUGS: ONLY ONE AT A TIME, first pass code
   This software may be freely distributed, no warranty.

   Changed to work with SIGALRM & SIGPROF by Tom Demmer.
   Gotchas:
     - It relies on uclock(), which does not work under Windows 3.X
       and sometimes under Windows 9X.
     - It screws up debuggers compiled with v2.02 or earlier, since
       debugging support before v2.03 couldn't pass signals to
       debugged programs.
     (Both of the above were true for the old version, too.)
*/

#include <libc/stubs.h>
#include <sys/time.h>
#include <sys/exceptn.h>
#include <errno.h>
#include <stdint.h>
#include <dpmi.h>
#include <signal.h>
#include <go32.h>


#define DEFAULT_CLOCK_TICK_INTERVAL 54926

/* Applications should set this to the number of microseconds between
   timer ticks if they reprogram the system clock.  */
long __djgpp_clock_tick_interval = -1;

static uclock_t r_exp, r_rel,  /* When REAL expires & reload value */
                p_exp, p_rel;  /* When PROF expires & reload value */

static uclock_t u_now;

/* Multiply a signed 32-bit VAL by a signed 32-bit M and divide the
   64-bit intermediate result by a signed 32-bit D.  The inline
   assembly avoids slow long long arithmetics.

   Originally written by Sergey Vlasov <vsu@au.ru>, with improvements
   by Nate Eldredge <neldredge@hmc.edu>.  */
static inline long
muldiv(long val, long m, long d)
{
  __asm__ __volatile__ ("imul %2\n\t"
			"idiv %3"
			: "=a" (val) : "0" (val), "rm" (m), "rm" (d) : "edx");
  return val;
}

int
getitimer(int which, struct itimerval *value)
{
  uclock_t expire, reload;

  u_now = uclock();
  if (which == ITIMER_REAL)
  {
    if (r_exp)
      expire = r_exp - u_now;
    else
      expire = 0;
    reload = r_rel;
  }
  else if (which == ITIMER_PROF)
  {
    if (p_exp)
      expire = p_exp - u_now;
    else
      expire = 0;
    reload = p_rel;
  }
  else
  {
    errno = EINVAL;
    return -1;
  }
  value->it_value.tv_sec    = expire / UCLOCKS_PER_SEC;
  value->it_value.tv_usec   = muldiv(expire % UCLOCKS_PER_SEC,
				     1000000, UCLOCKS_PER_SEC);
  value->it_interval.tv_sec = reload / UCLOCKS_PER_SEC;
  value->it_interval.tv_usec= muldiv(reload % UCLOCKS_PER_SEC,
				     1000000, UCLOCKS_PER_SEC);
  return 0;
}

static char timer_on = 0;

/* Set back IRQ2 handler to default values and disable own signal
   handler */
static void
stop_timer(void)
{
  if(!timer_on)
    return;
  __djgpp_timer_countdown = -1;
  __dpmi_set_protected_mode_interrupt_vector(8, &__djgpp_old_timer);
  timer_on = 0;
  signal(SIGTIMR, SIG_DFL);
}

/* Returns the time to the next event in UCLOCK_PER_SEC u_now must be
   set by calling routine.  Return 0 if no event pending. */

static inline uclock_t
GetNextEvent(void)
{
  if (r_exp && p_exp)
     return (r_exp < p_exp ? r_exp - u_now : p_exp - u_now );
  else if (r_exp)
     return  r_exp - u_now;
  else if (p_exp)
     return p_exp - u_now;
  else
     return 0;
}

/* Handler for SIGTIMR */
static void
timer_action(int signum)
{
  int do_tmr=0,do_prof=0;
  uclock_t next;

  u_now = uclock();

  /* Check the real timer */
  if (r_exp && (r_exp <= u_now) )
  {
    do_tmr = 1;
    if (r_rel)
      r_exp += r_rel;
    else
      r_exp = 0;
  }

  /* Check profile timer */
  if (p_exp && (p_exp <= u_now))
  {
    do_prof = 1;
    if (p_rel)
      p_exp += p_rel;
    else
      p_exp = 0;
  }

  /* Now we have to schedule the next interrupt, if any pending */
  if (do_tmr || do_prof)
  {
    if ((next = GetNextEvent()) != 0)
    {
      next /= 65536L;
      /* Why do I subtract 1 from NEXT below?  Because the timer
	 interrupt handler (see exceptn.S) checks whether the
	 countdown variable is zero *before* it decrements it.  So
	 setting it to zero means the timer will expire on the next
	 tick, which is exactly what we want.

	 Note also that NEXT might be negative if the timer just
	 fired, and if the reload value is smaller than u_now - X_exp.
	 We treat that as if NEXT were zero, meaning that the timer
	 will expire on the next tick.  */
      __djgpp_timer_countdown = next > 0 ? next - 1 : 0 ;
    }
    else
      stop_timer();

    if (do_tmr)
      raise(SIGALRM);
    if (do_prof)
      raise(SIGPROF);
  }
}

static void
start_timer(void)
{
  uclock_t next;
  __dpmi_paddr int8;

  next = GetNextEvent();
  next /= 65536L;
  /* See the commentary above about subtracting 1 from NEXT, and about
     negative values being returned by GetNextEvent.  */
  __djgpp_timer_countdown = next > 0 ? next - 1 : 0;

  if (timer_on)
    return;

  timer_on = 1;
  signal(SIGTIMR, timer_action);
  __dpmi_get_protected_mode_interrupt_vector(8, &__djgpp_old_timer);
  int8.selector = _my_cs();
  int8.offset32 = (uintptr_t) &__djgpp_timer_hdlr;
  __dpmi_set_protected_mode_interrupt_vector(8, &int8);
}


int
setitimer(int which, struct itimerval *value, struct itimerval *ovalue)
{
  uclock_t *t_exp, *t_rel;
  long usecs, usecs_min;

  if (ovalue)
  {
    if (getitimer(which,ovalue)) /* also sets u_now */
      return -1;  /* errno already set */
  }
  else
    u_now = uclock();

  if ((which != ITIMER_REAL) && ( which != ITIMER_PROF ) )
  {
    errno = EINVAL;
    return -1;
  }

  /* If VALUE is a NULL pointer, don't crash, just return the
     current timer value.  Posix systems seem to expect that behavior.  */
  if (!value)
    return 0;

  t_exp = which == ITIMER_REAL ? &r_exp: &p_exp;
  t_rel = which == ITIMER_REAL ? &r_rel: &p_rel;

  if ((value->it_value.tv_sec|value->it_value.tv_usec)==0 )
  {
    /* Disable this timer */
    *t_exp = *t_rel = 0;
    /* If both stopped, stop timer */
    if (( p_exp | r_exp ) == 0 )
      stop_timer();

    /* Even though it_value is zero, we need to record the interval,
       so don't return just yet.  */
  }

  *t_rel = (uclock_t)value->it_interval.tv_sec * UCLOCKS_PER_SEC;

  /* Posix systems expect timer values smaller than the resolution of
     the system clock be rounded up to the clock resolution.  */
  usecs_min = __djgpp_clock_tick_interval;
  if (usecs_min < 0)
    usecs_min = DEFAULT_CLOCK_TICK_INTERVAL;
  usecs = value->it_interval.tv_usec;
  if (value->it_interval.tv_sec == 0 && usecs && usecs < usecs_min)
    usecs = usecs_min;

  /* This doesn't overflow and doesn't cause any rounding errors,
     since the intermediate result inside muldiv is 64-bit wide. */
  *t_rel += muldiv(usecs, UCLOCKS_PER_SEC, 1000000);

  if ((value->it_value.tv_sec|value->it_value.tv_usec) == 0)
    return 0;

  *t_exp = (uclock_t)value->it_value.tv_sec * UCLOCKS_PER_SEC;
  usecs = value->it_value.tv_usec;
  if (value->it_value.tv_sec == 0 && usecs < usecs_min)
    usecs = usecs_min;
  *t_exp += muldiv(usecs, UCLOCKS_PER_SEC, 1000000);

  /* u_now is returned zero first time uclock() is called.  That first
     call could be the one we issued above, or it could be two days
     ago, when the calling program started.  We need to make {rp}_exp
     and u_now be relative to the same point of origin.  */
  *t_exp += u_now;

  start_timer();
  return 0;
}
