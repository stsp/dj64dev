/* Copyright (C) 2018 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2011 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <process.h>
#include <go32.h>
#include <dpmi.h>
#include <ctype.h>
#include <dos.h>
#include <sys/system.h>
#include <sys/movedata.h>
#include <libc/bss.h>
#include <libc/dosexec.h>
#include <libc/unconst.h>
#include <libc/dosio.h>
#include <libc/farptrgs.h>
#include <libc/symlink.h>

/* Maximum length of the command-line tail Int 21h/AH=4Bh can handle.  */
#define CMDLEN_LIMIT 126

/* Maximum length of the command we can pass through CMDLINE=.  */
#define CMDLINE_MAX  1023

/* Enable attempt to reallocate temporary transfer buffer for long command
   lines when defined */
#define __ENABLE_TB_REALLOC

extern char **_environ;

int __dosexec_in_system = 0;
int __spawn_flags = __spawn_leak_workaround;

typedef struct {
  unsigned short eseg;
  unsigned short argoff;
  unsigned short argseg;
  unsigned short fcb1_off;
  unsigned short fcb1_seg;
  unsigned short fcb2_off;
  unsigned short fcb2_seg;
} Execp;

static Execp parm;

static unsigned long tbuf_ptr;
static unsigned long tbuf_beg;
static unsigned long tbuf_end;
static unsigned long tbuf_len;
#ifdef __ENABLE_TB_REALLOC
static int           tbuf_selector;
#endif

static int script_exec(const char *, char **, char **);

/* Allocate AMT bytes off the transfer buffer.  */
static unsigned long talloc(size_t amt)
{
  unsigned long rv = tbuf_ptr;
  tbuf_ptr += amt;
  return rv;
}

/* Make sure we can allocate AMT bytes off the transfer buffer
   without overflowing it.  Return non-zero if we can, zero otherwise.

   WARNING: This can relocate the data already in the transfer buffer,
	    so all linear addresses which use it should be relative to
	    TBUF_BEG!  */
static int check_talloc(size_t amt)
{
  int retval = 1;

  if (tbuf_ptr + amt > tbuf_end)
  {
#ifdef __ENABLE_TB_REALLOC
    /* Code that reallocs the transfer buffer; currently disabled.  */
    unsigned long new_tb;
    unsigned long min_len = tbuf_len + amt;
    unsigned long max_len = 0x10000; /* 64KB */
    int old_selector = tbuf_selector;
    int max_avail;
    int e = errno;

    errno = E2BIG;

    /* Try to allocate new, larger DOS buffer, upto 64KB.  */
    if (min_len > max_len)
    {
      retval = 0;
      goto done;
    }
    while (tbuf_len <= max_len && tbuf_len < min_len)
      tbuf_len *= 2;
    if (tbuf_len < min_len)
    {
      retval = 0;
      goto done;
    }

    /* There is no use to allocate buffer bigger that 64K */
    if (tbuf_len > max_len)
      tbuf_len = max_len;

    tbuf_len = (tbuf_len + 15) & 0xffff0; /* round to nearest paragraph */

    if ((new_tb =
	 __dpmi_allocate_dos_memory(tbuf_len / 16, &max_avail)) == (unsigned)-1)
    {
      if (max_avail*16 < (int) min_len
	  || (new_tb =
	      __dpmi_allocate_dos_memory(max_avail, &tbuf_selector)) == (unsigned)-1)
      {
	retval = 0;
	goto done;
      }
      tbuf_len = max_avail * 16;
    }
    else
      tbuf_selector = max_avail;

    new_tb *= 16;  /* convert to linear address */
    fmemcpy12(DP(_dos_ds, new_tb), DP(_dos_ds, tbuf_beg), tbuf_ptr - tbuf_beg);
    tbuf_ptr = new_tb + tbuf_ptr - tbuf_beg;
    tbuf_beg = new_tb;
    tbuf_end = tbuf_beg + tbuf_len - 1;

    errno = e;

done:
    /* Assume caller will return immediately in case of
       failure to reallocate, so they won't need the old data.  */
    if (!retval)
      tbuf_selector = 0;
    if (old_selector)
      __dpmi_free_dos_memory(old_selector);
#else
    errno = E2BIG;
    retval = 0;
#endif
  }

  return retval;
}

extern char   __PROXY[];	/* defined on crt0/crt1.c */
extern size_t __PROXY_LEN;

char __cmdline_str[] = "CMDLINE=";
size_t __cmdline_str_len = sizeof(__cmdline_str) - 1;

/* Functions that call `direct_exec_tail' after they've put
   some data into the transfer buffer, should set LFN parameter
   to either 0 (no LFN support) or 1 (LFN supported), but NOT 2!
   if LFN is 2, there is a possiblity that the contents of the
   transfer buffer will be overrun!  */
static int
direct_exec_tail_1(const char *program, const char *args,
                   char * const envp[], const char *proxy, int lfn,
                   const char *cmdline_var)
{
  __dpmi_regs r = {0};
  unsigned long program_la;
  unsigned long arg_la;
  unsigned long parm_la;
  unsigned long env_la, env_e_la;
#ifdef __ENABLE_TB_REALLOC
  unsigned long proxy_off = 0;
  int initial_tbuf_selector = tbuf_selector;
#endif
  size_t proxy_len = proxy ? strlen(proxy) + 1 : 0;
  int seen_proxy = 0, seen_cmdline = 0;
  char arg_header[3];
  char short_name[FILENAME_MAX];
  const char *progname;
  unsigned proglen;
  int i;
  unsigned long fcb1_la, fcb2_la, fname_la;
  int arg_len;
  int long_args_via_cmdline = 0;
  /* The funky +1 below is because we need to copy the environment
     variables with their terminating null characters.  */
  size_t cmdline_var_len = cmdline_var ? strlen(cmdline_var) + 1 : 0;

  /* This used to just call sync().  But `sync' flushes the disk
     cache nowadays, and that can slow down the child tremendously,
     since some caches (e.g. SmartDrv) invalidate all of their
     buffers when `_flush_disk_cache' is called.  */
  for (i = 0; i < 255; i++)
    fsync(i);

  if (lfn == 2)		/* don't know yet */
    lfn = _USE_LFN;

  /* The pathname of the executable to run.  */
  proglen = strlen(program) + 1;
  if (!check_talloc(proglen))
    return -1;
  /* Make sure any magic names, like /dev/c/foo, are converted to the
     usual DOS form, and, under LFN, to the short 8+3 alias.  */
  _put_path2(program, tbuf_beg == __tb ? tbuf_ptr - tbuf_beg : 0);
  if (lfn)
  {
    unsigned pgm_name_loc = tbuf_beg == __tb ? tbuf_ptr : __tb;
    r.x.flags = 1;			/* Always set CF before calling a 0x71NN function. */
    r.x.ax = 0x7160;			/* Truename */
    r.x.cx = 1;				/* Get short name */
    r.x.ds = r.x.es = pgm_name_loc / 16;
    r.x.si = r.x.di = pgm_name_loc & 15;
    __dpmi_int(0x21, &r);
    if ((r.x.flags & 1) || (r.x.ax == 0x7100))
    {
      /*  Never assume that the complete LFN API is implemented,
          so check that AX != 0x7100.  E.G.: MSDOS 6.22 and DOSLFN 0.40.
          If not supported fail.  */
      errno = __doserr_to_errno(r.x.ax);
      return -1;
    }
  }
  dosmemget(tbuf_beg == __tb ? tbuf_ptr : __tb, FILENAME_MAX, short_name);
  progname = short_name;
  proglen = strlen(short_name) + 1;

  if (!check_talloc(proglen + strlen(args) + 3 + sizeof(Execp) + 48))
    return -1;
  program_la = talloc(proglen);
  arg_la     = talloc(strlen(args) + 3);
  parm_la    = talloc(sizeof(Execp));

  dosmemput(progname, proglen, program_la);

  /* The command-line tail.  */
  arg_len = strlen(args);
  if (arg_len > CMDLEN_LIMIT)
  {
    arg_len = CMDLEN_LIMIT;
    if (cmdline_var)
    {
      arg_header[0] = 127;
      long_args_via_cmdline = 1;
    }
    else
      arg_header[0] = CMDLEN_LIMIT; /* truncate the command tail */
  }
  else
    arg_header[0] = arg_len;
  arg_header[1] = '\r';

  dosmemput(arg_header, 1, arg_la); /* command tail length byte */
  dosmemput(args, arg_len, arg_la + 1); /* command tail itself */
  dosmemput(arg_header + 1, 1, arg_la + 1 + arg_len); /* terminating CR */

#ifdef __ENABLE_TB_REALLOC
  if (strncmp(args, __PROXY, __PROXY_LEN) == 0 && args[__PROXY_LEN] == ' ')
     proxy_off = arg_la + 1 - tbuf_beg;
#endif

  /* The 2 FCBs.  Some programs (like XCOPY from DOS 6.x) need them.  */
  fcb1_la = talloc(16);        /* allocate space for 1st FCB */
  fname_la = arg_la + 1;       /* first character of command tail */
  r.x.ax = 0x2901;             /* AL = 1 means skip leading separators */
  r.x.ds = fname_la / 16;      /* pointer to 1st cmd argument */
  r.x.si = fname_la & 15;
  r.x.es = fcb1_la / 16;       /* pointer to FCB buffer */
  r.x.di = fcb1_la & 15;
  __dpmi_int (0x21, &r);

  /* We cannot be sure that Int 21h/AX=2901h parsed the entire
     first command-line argument (it might not be a filename
     at all!).  We need to get to the next command-line arg
     before calling 2901 again.  2901 returns the pointer to
     first unparsed character in DS:SI.

     Note that, in case there is no second command-line argument,
     the following loop is terminated by the trailing CR which
     ends the command-line tail.  */
  for (_farsetsel(_dos_ds), fname_la = ((unsigned)r.x.ds) * 16 + r.x.si;
       !isspace(_farnspeekb(fname_la));
       fname_la++)
    ;

  fcb2_la = talloc(16);
  r.x.ax = 0x2901;
  r.x.ds = fname_la / 16;      /* begin parsing 2nd arg from here */
  r.x.si = fname_la & 15;
  r.x.es = fcb2_la / 16;
  r.x.di = fcb2_la & 15;
  __dpmi_int (0x21, &r);

  /* The environment must be on a segment boundary, so get
     to the first location in the transfer buffer whose
     linear address is divisable by 16.  */
  do {
    env_la = talloc(1);
  } while (env_la & 15);
  talloc(-1);

#ifdef __ENABLE_TB_REALLOC
  /* Convert to relative, since `check_talloc' may relocate.  */
  arg_la  -= tbuf_beg;
  env_la  -= tbuf_beg;
  fcb1_la -= tbuf_beg;
  fcb2_la -= tbuf_beg;
  parm_la -= tbuf_beg;
  program_la -= tbuf_beg;
#endif

  /* The environment.  Replace the !proxy variable, if there is
     one (for nested programs) if we are called from `system',
     or skip it, if we are called from `spawnXX'.
     Similar treatment is given the CMDLINE variable.  */
  for (i = 0; envp[i]; i++)
  {
#ifdef __ENABLE_TB_REALLOC
    int have_proxy = 0;
#endif
    const char *ep = envp[i];
    size_t env_len = strlen(ep) + 1;

    if (strncmp(ep, __PROXY, __PROXY_LEN) == 0 && ep[__PROXY_LEN] == '=')
    {
      seen_proxy = 1;
      if (proxy)
      {
	ep = proxy;
	env_len = proxy_len;
#ifdef __ENABLE_TB_REALLOC
        have_proxy = 1;
#endif
      }
      else
	continue;
    }
    else if (long_args_via_cmdline &&
             strncmp(ep, __cmdline_str, __cmdline_str_len) == 0)
    {
      seen_cmdline = 1;
      ep = cmdline_var;
      env_len = cmdline_var_len;
    }
    if (!check_talloc(env_len))
      return -1;
    env_e_la = talloc(env_len);
#ifdef __ENABLE_TB_REALLOC
    if (have_proxy) proxy_off = env_e_la - tbuf_beg;
#endif
    dosmemput(ep, env_len, env_e_la);
  }

  /* If no !proxy variable was found, create one.  */
  if (proxy && !seen_proxy)
  {
    if (!check_talloc(proxy_len))
      return -1;
    env_e_la = talloc(proxy_len);
#ifdef __ENABLE_TB_REALLOC
    proxy_off = env_e_la - tbuf_beg;
#endif
    dosmemput(proxy, proxy_len, env_e_la);
  }

  /* If no CMDLINE variable was found, and we need it, create one.  */
  if (long_args_via_cmdline && !seen_cmdline)
  {
    if (!check_talloc(cmdline_var_len))
      return -1;
    env_e_la = talloc(cmdline_var_len);
    dosmemput(cmdline_var, cmdline_var_len, env_e_la);
  }

  /* Terminate by an extra NULL char.  */
  arg_header[0] = 0;

  /* The name of the program that owns the environment.  */
  arg_header[1] = 1;	/* the number of strings (1, little-endian) */
  arg_header[2] = 0;
  if (!check_talloc(3 + proglen))
    return -1;
  dosmemput(arg_header, 3, talloc(3));
  env_e_la = talloc(proglen);
  dosmemput(progname, proglen, env_e_la);

  /* Prepare the parameter block and call Int 21h/AX=4B00h.  */
#ifdef __ENABLE_TB_REALLOC
  arg_la  += tbuf_beg;
  env_la  += tbuf_beg;
  fcb1_la += tbuf_beg;
  fcb2_la += tbuf_beg;
  parm_la += tbuf_beg;
  program_la += tbuf_beg;

  if ((initial_tbuf_selector != tbuf_selector) && proxy_off)
  {
    char temp[65], *s, t2[5];
    sprintf(t2, "%04X", (unsigned short)(tbuf_beg >> 4));
    dosmemget(tbuf_beg + proxy_off, 64, temp);
    temp[64] = 0;
    s = strchr(temp, '\r');
    if (s) *s = 0;
    dosmemput(t2, 4, tbuf_beg + proxy_off + 13);
    if (strlen(temp) > 23)
      dosmemput(t2, 4, tbuf_beg + proxy_off + 23);
  }
#endif

  parm.eseg     = env_la / 16;
  parm.argseg	= arg_la / 16;
  parm.argoff	= arg_la & 15;
  parm.fcb1_seg = fcb1_la / 16;
  parm.fcb1_off = fcb1_la & 15;
  parm.fcb2_seg = fcb2_la / 16;
  parm.fcb2_off = fcb2_la & 15;
  dosmemput(&parm, sizeof(parm), parm_la);

  r.x.ax = 0x4B00;
  r.x.ds = program_la / 16;
  r.x.dx = program_la & 15;
  r.x.es = parm_la / 16;
  r.x.bx = parm_la & 15;
  __dpmi_int(0x21, &r);
#ifdef __ENABLE_TB_REALLOC
  if (tbuf_selector)
    __dpmi_free_dos_memory(tbuf_selector);
  tbuf_selector = 0;
#endif
#if 0
  /* Work around the W2K NTVDM bug; see dpmiexcp.c for detailed
     explanations.  */
  __maybe_fix_w2k_ntvdm_bug();
#endif

  if (r.x.flags & 1)
  {
    errno = __doserr_to_errno(r.x.ax);
    return -1;
  }

  r.h.ah = 0x4d;
  __dpmi_int(0x21, &r);

  if (r.x.flags & 1)
  {
    errno = __doserr_to_errno(r.x.ax);
    return -1;
  }

  /* AH holds the ``system exit code'' which is non-zero if the
     child was aborted by Ctrl-C, or Critical Device error (also
     if the child installs itself as a TSR).  */
  if (r.h.ah && r.h.ah != 3) /* 3 means it exited as TSR (is it ``normal''?) */
  {
    errno = EINTR;	/* what else can we put in `errno'? */
    return (((r.h.ah == 1 ? SIGINT : SIGABRT) << 8) | r.h.al);
  }
  return r.h.al;	/* AL holds the child exit code */
}

static int direct_exec_tail (const char *program, const char *args,
		 char * const envp[], const char *proxy, int lfn,
		 const char *cmdline_var)
{
  int i, ret;
  int sel1, sel2;
  static char first_call = 1;
  static char workaround_descriptor_leaks = 0;
  static unsigned char larbyte = 0;
  static int how_deep = 100;		/* Tunable parameter */
  char desc_map[how_deep];

  sel1 = sel2 = 0;			/* Clear warnings */

  if (first_call)			/* One time algorithm detection */
  {
    int flags;
    char dpmi_vendor[128];
    unsigned char desc_buf1[8], desc_buf2[8];

    /* Disable descriptors leak workaround when CWSDPMI is being used */
    /* since not needed.  Does not work with CWSDPMI versions before  */
    /* r5 as corresponding DPMI call is supported beginning with v5.  */

    ret = __dpmi_get_capabilities(&flags, dpmi_vendor);
    if ((ret == 0 && strcmp(dpmi_vendor + 2, "CWSDPMI") == 0)
        || (__spawn_flags & __spawn_leak_workaround) == 0)
      workaround_descriptor_leaks = 0;
    else
    {
      /* Test the DPMI provider to see how it behaves.  We allocate
         a descriptor, get its access rights and full 8 byte descriptor.
         We then free it, and see what changed.  The present bit, run
         ring and selector type (user) might change.  Algorithm 1: These
         can be detected with a single hardware instruction (LAR).
         __dpmi_get_descriptor_access_rights is actually not an interrupt
         but just a wrapper around this instruction.  However, Win NT/2K
         do something strange on the LAR, so once a selector is allocated
         it stays visable.  Algorithm 2: Get the full descriptor and
         compare extracted LAR nibble.  If the get descriptor call fails
         we also know this descriptor is currently not user allocated. */

      sel1 = __dpmi_allocate_ldt_descriptors(1);
      larbyte = 0xf0 & __dpmi_get_descriptor_access_rights(sel1);
      __dpmi_get_descriptor(sel1, desc_buf1);
      __dpmi_free_ldt_descriptor(sel1);
      flags = __dpmi_get_descriptor_access_rights(sel1);	/* freed */

      if (larbyte != (flags & 0xf0))
        workaround_descriptor_leaks = 1;	/* present+ring+sys changed */
      else
      {				/* Win NT/2K/XP lie about lar */
        larbyte = desc_buf1[5] & 0xf0;
        ret = __dpmi_get_descriptor(sel1, desc_buf2);
        if (ret == -1 || (larbyte != (desc_buf2[5] & 0xf0)))
          workaround_descriptor_leaks = 2;
        else
          workaround_descriptor_leaks = 0;	/* Don't do anything */
      }
    }
    first_call = 0;
  }

  if (workaround_descriptor_leaks)		/* Create the unused map */
  {
    unsigned char desc_buf[8];
    char *map = desc_map;

    /* Create a map of the free descriptors in the probable range the
       children will use.  We start by allocating a descriptor, assuming
       it will be close to the start of what the children will use.  We
       then scan an area "how_deep" beyond that.  Since a DJGPP app will
       leak 4 descriptors if it's all cleaned up, we allocate 25 times that
       much - maybe one of them is before this fix.  We use the algorithm
       determined in the one pass identification. */

    sel1 = __dpmi_allocate_ldt_descriptors(1);
    if (sel1 < _dos_ds)
    {                                /* Failure -1 also matches */
      sel1 = _dos_ds;                /* Our last descriptor */
      *map++ = 0;                    /* don't free it */
    }
    else
      *map++ = 1;                    /* sel1 always released */
    sel2 = sel1 + 8 * how_deep - 8;  /* end of scan range inclusive */

    if (workaround_descriptor_leaks == 1)
    {
      for (i = sel1 + 8; i <= sel2; i += 8)
        *map++ = (__dpmi_get_descriptor_access_rights(i) & 0xf0) != larbyte;
    }
    else if (workaround_descriptor_leaks == 2)
    {
      for (i = sel1 + 8; i <= sel2; i += 8)
        if ((__dpmi_get_descriptor_access_rights(i) & 0xf0) != larbyte)
          *map++ = 1;                /* Never touched, free it */
        else if (__dpmi_get_descriptor(i, desc_buf) == -1)
          *map++ = 1;                /* Means free on NT */
        else
          *map++ = (desc_buf[5] & 0xf0) != larbyte;
    }
  }

  ret = direct_exec_tail_1(program, args, envp, proxy, lfn, cmdline_var);

  if (workaround_descriptor_leaks)   /* Free the unused map */
  {
    int endsel;
    char *map;

    /* First, allocate a selector to estimate the end of the leaked range.
       This is only used to determine if our "how_deep" range was adequate.
       Use the map of the free descriptors created before the children
       were spawned.  For algorithm 1 since the LAR instruction is fast
       we check first to see if the selector is allocated before deallocating.
       For algorithm 2, the check is a DPMI call and slow, so we just free
       the descriptors which were free before we called the children and
       touched according to the LAR byte.  Finally, if the "how_deep"
       range was too small, we permanently lose the extra selectors, but
       we double the range beyond what we saw this time.  If we lose a few
       it's the price we pay for better performance.  */

    map = desc_map + how_deep;

    endsel = __dpmi_allocate_ldt_descriptors(1);
    __dpmi_free_ldt_descriptor (endsel);

    if (workaround_descriptor_leaks)
    {							/* Algorithms 1 & 2 */
      for (i = sel2; i >= sel1; i -= 8)
        if (*--map)
          if ((__dpmi_get_descriptor_access_rights(i) & 0xf0) == larbyte)
            __dpmi_free_ldt_descriptor(i);
    }

    if (endsel > sel2)
    {
      how_deep = (endsel - sel1) / 4;			/* Twice what's needed */
#if 0
      for (i = endsel - 8; i >= sel2; i -= 8)		/* Unsafe free, no map */
        __dpmi_free_ldt_descriptor(i);
#endif
    }
  }
  return ret;
}

int
_dos_exec(const char *program, const char *args, char * const envp[],
	  const char *cmdline_var)
{
  tbuf_beg = tbuf_ptr = __tb;
  tbuf_len = __tb_size;
  tbuf_end = tbuf_beg + tbuf_len - 1;
  return direct_exec_tail(program, args, envp, 0, 2, cmdline_var);
}

static char GO32_V2_STRING[] = "go32-v2.exe";
static char GO32_STRING[]    = "go32.exe";

struct __shell_data {
  const char *name;		/* shell basename */
  unsigned int cmdline_limit;	/* command-line length limit; -1 if no limit */
};

/* A list of known shells which require we DON'T quote command
   lines that are passed to them with the /c or -c switch.  */
static struct __shell_data shell_brokets[] = {
  { "COMMAND.COM", (unsigned int)-2 },	/* -2 is special, see below */
  { "4DOS.COM",    255 },
  { "NDOS.COM",    255 },
  { 0,             0 }
};

/* A list of Unix-like shells and other non-DJGPP programs
   which treat single quote specially.  */
static struct __shell_data unix_shells[] = {
  { "SH.EXE",    (unsigned int)-1 },
  /* People create `-sh.exe' and `-bash.exe' to have login shells.  */
  { "-SH.EXE",   (unsigned int)-1 },
  { "SH16.EXE",  CMDLEN_LIMIT }, /* assuming Stewartson's ms_sh or its ilk */
  { "SH32.EXE",  CMDLEN_LIMIT },
  { "TCSH.EXE",  CMDLINE_MAX }, /* assuming a Windows port */
  { "-TCSH.EXE", CMDLINE_MAX },
  { "BASH.EXE",  (unsigned int)-1 },
  { "-BASH.EXE", (unsigned int)-1 },
  { 0, 0}
};

unsigned int
_shell_cmdline_limit(const char *program)
{
  const char *p = program, *ptail = program;
  struct __shell_data *program_list;
  int i;

  while (*p)
  {
    if (*p == '/' || *p == ':' || *p == '\\')
      ptail = p + 1;
    p++;
  }

  for (i = 0, program_list = shell_brokets; program_list[i].name; i++)
    if (!stricmp (ptail, program_list[i].name))
    {
      if (program_list[i].cmdline_limit == (unsigned int)-2)
      {
	if (_osmajor >= 7 && _osmajor < 10) /* OS/2 reports v10 */
	  return (unsigned int)-1;
	return CMDLEN_LIMIT;
      }
      return program_list[i].cmdline_limit;
    }

  for (i = 0, program_list = unix_shells; program_list[i].name; i++)
    if (!stricmp (ptail, program_list[i].name))
    {
      if (program_list[i].cmdline_limit == (unsigned int)-2)
      {
	/* FIXME: Do we need to see if we run on Windows?  */
	if (_osmajor >= 7
	    && _osmajor < 10  /* OS/2 reports v10 */
	    && stricmp(_os_flavor, "ms-dos") == 0)
	  return CMDLINE_MAX;
	return CMDLEN_LIMIT;
      }
      return program_list[i].cmdline_limit;
    }

  return 0;
}

static int
list_member(const char *program, struct __shell_data program_list[])
{
  const char *p = program, *ptail = program;
  int i;

  while (*p)
  {
    if (*p == '/' || *p == ':' || *p == '\\')
      ptail = p + 1;
    p++;
  }

  for (i = 0; program_list[i].name; i++)
    if (!stricmp (ptail, program_list[i].name))
      return 1;

  return 0;
}

int
_is_unixy_shell(const char *shellpath)
{
  return list_member(shellpath, unix_shells);
}

int
_is_dos_shell(const char *shellpath)
{
  return list_member(shellpath, shell_brokets);
}

static int direct_pe_exec(const char *, char **, char **);

static int direct_exec(const char *program, char **argv, char **envp)
{
  int i;
  char args[CMDLEN_LIMIT + 1], *argp = args;
  int need_quote = !__dosexec_in_system;
  int unescape_quote = __dosexec_in_system;
  int dos_shell = _is_dos_shell(program);
  int unix_shell = _is_unixy_shell(program);

  /* Shells on Windows are normally EXE-style programs (even
     COMMAND.COM is a .EXE program in disguise: it has the telltale MZ
     signature).  So they usually will end up in direct_exec, unless
     they are DJGPP programs.  However, direct_exec is limited to
     126-char command lines.  So we pass shell commands to
     direct_pe_exec, which can handle longer commands.  */
  if ((dos_shell || unix_shell)
      && _shell_cmdline_limit(program) > (unsigned) CMDLEN_LIMIT)
    return direct_pe_exec(program, argv, envp);

  /* PROGRAM can be a shell which expects a single argument
     (beyond the /c or -c switch) that is the entire command
     line.  With some shells, we must NOT quote that command
     line, because that will confuse the shell.

     The hard problem is to know when PROGRAM names a shell
     that doesn't like its command line quoted...  */

  if (need_quote
      && argv[1] && !strcmp (argv[1], "/c")
      && argv[2] && !argv[3]
      && dos_shell)
    need_quote = 0;

  if (unescape_quote && unix_shell)
    unescape_quote = 0;

  for (i = 1; argv[i]; i++)
  {
    int quoted = 0;
    const char *p = argv[i];

    if (argp - args > CMDLEN_LIMIT)
      break;
    *argp++ = ' ';
    /* If invoked by `spawnXX' or `execXX' functions, we need to
       quote arguments which include whitespace, so they end up
       as a single argument on the child side.
       We will invoke PROGRAM directly by DOS Exec function (not
       through COMMAND.COM), therefore no need to quote characters
       special only to COMMAND.COM.
       We also assume that DJGPP programs aren't invoked through
       here, so a single quote `\'' is also not special.  The only
       programs other than DJGPP that treat a single quote specially
       are Unix-like shells, but whoever uses them should know to
       escape the quotes himself.  */
    if (need_quote && strpbrk(p, " \t") != 0)
    {
      if (argp - args > CMDLEN_LIMIT)
	break;
      *argp++ = '"';
      quoted = 1;
    }
    while (*p)
    {
      if (argp - args > CMDLEN_LIMIT)
        break;
      if (*p == '"' && (quoted || need_quote))
	*argp++ = '\\';
      /* Most non-DJGPP programs don't treat `\'' specially,
	 but our `system' requires we always escape it, so
	 we should undo the quoting here.  */
      else if (*p == '\\' && p[1] == '\'' && unescape_quote)
	p++;
      if (argp - args > CMDLEN_LIMIT)
        break;
      *argp++ = *p++;
    }
    if (quoted && argp - args <= CMDLEN_LIMIT)
      *argp++ = '"';
  }
  *argp = 0;

  if (argp - args > CMDLEN_LIMIT)
    errno = E2BIG;

  tbuf_beg = tbuf_ptr = __tb;
  tbuf_len = __tb_size;
  tbuf_end = tbuf_beg + tbuf_len - 1;
  return direct_exec_tail(program, args, envp, 0, 2, 0);
}

static int direct_pe_exec(const char *program, char **argv, char **envp)
{
  int i;
  size_t arglen;
  char *args, *argp, *varp, *vp;
  int need_quote = !__dosexec_in_system;
  int unescape_quote = __dosexec_in_system;
  size_t proglen = strlen(program);
  int dos_shell = _is_dos_shell (program);

  /* PROGRAM can be a shell which expects a single argument
     (beyond the /c or -c switch) that is the entire command
     line.  With some shells, we must NOT quote that command
     line, because that will confuse the shell.

     The hard problem is to know when PROGRAM names a shell
     that doesn't like its command line quoted...  */

  if (need_quote
      && argv[1] && !strcmp (argv[1], "/c")
      && argv[2] && !argv[3]
      && dos_shell)
    need_quote = 0;

  if (unescape_quote && _is_unixy_shell (program))
    unescape_quote = 0;

  arglen = 0;
  /* For each element in argv[], we allocate twice its length, for the
     extreme case where each character is a quote that needs to be
     escaped, plus 2 for two outer quotes, plus 1 for the delimiting
     blank.  */
  for (i = 1; argv[i]; i++)
    arglen += 2*strlen(argv[i]) + 1 + 2;

  /* Assumption: PROGRAM does not include quotes (DOS/Windows do not
     allow them in file names).  +2 is for possible quotes that we
     might need if PROGRAM includes whitespace characters.  */
  varp = (char *)alloca(__cmdline_str_len + proglen + 2 + arglen + 1);
  strcpy(varp, __cmdline_str);
  argp = varp + __cmdline_str_len;
  if (memchr(program, ' ', proglen) != NULL)
  {
    *argp++ = '"';
    memcpy(argp, program, proglen);
    argp += proglen;
    *argp++ = '"';
  }
  else
  {
    memcpy(argp, program, proglen);
    argp += proglen;
  }
  args = argp;	/* ARGS should point to the command tail */

  /* Non-DJGPP programs might not like forward slashes in their full
     path name.  */
  for (vp = varp + __cmdline_str_len; vp < argp; vp++)
    if (*vp == '/')
      *vp = '\\';

  for (i = 1; argv[i]; i++)
  {
    int quoted = 0;
    const char *p = argv[i];

    if (argp - varp > CMDLINE_MAX)
      break;
    *argp++ = ' ';
    /* If invoked by `spawnXX' or `execXX' functions, we need to
       quote arguments which include whitespace, so they end up
       as a single argument on the child side.
       We will invoke PROGRAM directly by DOS Exec function (not
       through COMMAND.COM), therefore no need to quote characters
       special only to COMMAND.COM.
       We also assume that DJGPP programs aren't invoked through
       here, so a single quote `\'' is also not special.  The only
       programs other than DJGPP that treat a single quote specially
       are Unix-like shells, but whoever uses them should know to
       escape the quotes himself.  */
    if (need_quote && strpbrk(p, " \t") != 0)
    {
      if (argp - varp > CMDLINE_MAX)
	break;
      *argp++ = '"';
      quoted = 1;
    }
    while (*p)
    {
      if (argp - varp > CMDLINE_MAX)
	break;
      if (*p == '"' && (quoted || need_quote))
	*argp++ = '\\';
      /* Most non-DJGPP programs don't treat `\'' specially,
	 but our `system' requires we always escape it, so
	 we should undo the quoting here.  */
      else if (*p == '\\' && p[1] == '\'' && unescape_quote)
	p++;
      if (argp - varp > CMDLINE_MAX)
	break;
      *argp++ = *p++;
    }
    if (quoted && argp - varp <= CMDLINE_MAX)
      *argp++ = '"';
  }
  *argp = 0;

  if (argp - varp > CMDLINE_MAX)
    errno = E2BIG;

  tbuf_beg = tbuf_ptr = __tb;
  tbuf_len = __tb_size;
  tbuf_end = tbuf_beg + tbuf_len - 1;

  /* If the command line is too long to pass directly, put the entire
     contents of the command line into the CMDLINE variable.
     direct_exec_tail will take care of the final details. */
  return direct_exec_tail(program, args, envp, 0, 2,
			  argp - args > CMDLEN_LIMIT ? varp : 0);
}

static int go32_exec(const char *program, char **argv, char **envp)
{
  const _v2_prog_type * type;
  char *save_argv0;
  int i;
  char *go32, *sip = 0;
  char rpath[FILENAME_MAX];
  char real_program[FILENAME_MAX];
  int argc = 0;

  int si_la = 0, si_off = 0, rm_off, argv_off;
  char cmdline[CMDLEN_LIMIT+2], *cmdp = cmdline;
  char *pcmd = cmdline, *pproxy = 0, *proxy_cmdline = 0;
  int lfn = 2;	/* means don't know yet */

  if (!__solve_symlinks(program, real_program))
     return -1;

  type = _check_v2_prog(real_program, -1);

  /* Because this function is called only, when program
     exists, I can skip the check for type->valid */

#define v2_0 (type->version.v.major > 1)
#define is_stubbed (type->exec_format == _V2_EXEC_FORMAT_STUBCOFF)
#define is_coff (type->object_format == _V2_OBJECT_FORMAT_COFF)
#define found_si (type->has_stubinfo)

  if (type->exec_format == _V2_EXEC_FORMAT_UNIXSCRIPT)
  {
    return script_exec(real_program, argv, envp);
  }

  /* Non-DJGPP programs cannot be run by !proxy.  */
  else if (!is_coff)
  {
    const char *ext = strrchr(real_program, '.');

    if (type->exec_format == _V2_EXEC_FORMAT_EXE
	|| (ext && stricmp(ext, ".com") == 0))
    {
      if (type->object_format != _V2_OBJECT_FORMAT_PE_COFF)
	return direct_exec(real_program, argv, envp);
      else
	return direct_pe_exec(real_program, argv, envp);
    }
    else
      return __dosexec_command_exec(real_program, argv, envp);
  }

  if (found_si)
    go32 = type->stubinfo->go32;
  else if (v2_0 && !is_stubbed)
    go32 = GO32_V2_STRING;
  else
    go32 = GO32_STRING;

  if (v2_0 && is_stubbed)
  {
    strcpy(rpath, real_program);
  }
  else
  {
    int e = errno;
    if (!__dosexec_find_on_path(go32, envp, rpath))
    {
      errno = e;
      return direct_exec(real_program, argv, envp); /* give up and just run it */
    }

    if (found_si)
    {
      sip = (char *)type->stubinfo;
    }
  }

  /* V2.0 programs invoked by `system' must be run via
     `direct_exec', because otherwise the command-line arguments
     won't be globbed correctly by the child.  Only v2.01 and
     later knows how to get long command lines from `system' AND
     glob them correctly.  But we don't want to check with which
     version was the child compiled, so we need to create both the
     usual DOS command line and the !proxy one (which will be put
     into the environment).  Sigh...  */
  save_argv0 = argv[0];
  /* Since that's where we really found it */
  argv[0] = unconst(program, char *);
  /* Construct the DOS command tail */
  for (argc = 0; argv[argc]; argc++);

  if (__dosexec_in_system && v2_0)
  {
    /* If PROGRAM is an un-stubbed COFF, its name must be passed
       in the command tail as well, since we call GO32 to run it.  */
    for (i = (is_stubbed ? 1 : 0); i < argc; i++)
    {
      const char *p = argv[i];
      if (cmdp - cmdline > CMDLEN_LIMIT)
	break;
      *cmdp++ = ' ';
      while (*p)
      {
	if (cmdp - cmdline > CMDLEN_LIMIT)
	  break;
	*cmdp++ = *p++;
      }
    }
    *cmdp = '\0';
  }

  lfn = _USE_LFN;

  /* Can't call any functions that use the transfer buffer beyond
     this point: they will overwrite the data already in __tb.  */

  tbuf_beg = tbuf_ptr = __tb;
  tbuf_len = __tb_size;
  tbuf_end = tbuf_ptr + tbuf_len - 1;

  /* Starting from DJGPP v2.04, programs are always run through !proxy.
     This allows correctly handle symlinks to .exes. */
  if (!check_talloc(found_si ?
		    type->stubinfo->struct_length : 0
		    + (argc+1)*sizeof(short)))
  {
    argv[0] = save_argv0;
    return -1;
  }
  if (found_si)
  {
    si_la = talloc(type->stubinfo->struct_length);
    si_off = si_la - tbuf_beg;
    dosmemput(sip, type->stubinfo->struct_length, si_la);
  }

  rm_off = argv_off = talloc((argc+1) * sizeof(short)) - tbuf_beg;
#if 0
  /* `alloca' could be dangerous with long command lines.  We
     will instead move the offsets one by one with `_farpokew'.  */
  rm_argv = (short *)alloca((argc+1) * sizeof(short));
#endif

  for (i = 0; i < argc; i++)
  {
    char *pargv = argv[i];
    int sl = strlen(pargv) + 1;
    unsigned long q;

    if (check_talloc(sl))
    {
      q = talloc(sl);
      dosmemput(pargv, sl, q);
      _farpokew(_dos_ds, tbuf_beg + argv_off, (q - tbuf_beg) & 0xffff);
      argv_off += sizeof(short);
    }
    else	/* not enough space to pass args */
    {
      argv[0] = save_argv0;
      return -1;
    }
  }

  _farpokew(_dos_ds, tbuf_beg + argv_off, 0);
  argv_off += sizeof(short);

  argv[0] = save_argv0;
  proxy_cmdline = (char *)alloca (34);

  sprintf(proxy_cmdline, "%s=%04x %04x %04x %04x %04x",
    __PROXY, (unsigned short)argc,
    (unsigned short)(tbuf_beg >> 4), (unsigned short)(rm_off & 0xffff),
    (unsigned short)(tbuf_beg >> 4), (unsigned short)(si_off & 0xffff));
  if (!found_si)
    proxy_cmdline[22] = 0; /* remove stubinfo information */

  if (__dosexec_in_system && v2_0)
    pproxy = proxy_cmdline;
  else
  {
    /* `proxy_cmdline looks like an environment variable " !proxy=value".
        This is used as the REAL command line specification by 2.01
        and later executables when called by `system'.  But if that's
        not the case, we need a blank instead of the `='.  */
    proxy_cmdline[__PROXY_LEN] = ' ';
    pcmd = proxy_cmdline;
  }

  return direct_exec_tail(rpath, pcmd, envp, pproxy, lfn, 0);
}

int
__dosexec_command_exec(const char *program, char **argv, char **envp)
{
  const char *comspec = 0;
  char *cmdline;
  int cmdlen;
  int i;
  int was_quoted = 0;	/* was the program name quoted? */
  char real_program[FILENAME_MAX];
  size_t cmdline_len;
  char *cmdline_var = NULL;

  if (!__solve_symlinks(program, real_program))
     return -1;

  /* Add spare space for possible quote characters.  */
  cmdlen = strlen(real_program) + 4 + 2;
  for (i = 0; argv[i]; i++)
    cmdlen += 2 * strlen(argv[i]) + 1;
  cmdline = (char *)alloca(cmdlen);

  strcpy(cmdline, "/c ");
  if (strchr(real_program, ' ') || strchr(real_program, '\t'))
  {
    was_quoted = 1;
    cmdline[3] = '"';
  }
  for (i = 0; real_program[i] > ' '; i++)
  {
    /* COMMAND.COM cannot grok program names with forward slashes.  */
    if (program[i] == '/')
      cmdline[i + 3 + was_quoted] = '\\';
    else
      cmdline[i + 3 + was_quoted] = real_program[i];
  }
  for (; real_program[i]; i++)
    cmdline[i + 3 + was_quoted] = real_program[i];
  if (was_quoted)
  {
    cmdline[i + 3 + was_quoted] = '"';
    i++;
  }
  cmdline[i + 3 + was_quoted] = 0;
  for (i = 1; argv[i]; i++)
  {
    strcat(cmdline, " ");
    /* If called by `spawnXX' or `execXX' functions, must quote
       arguments that have embedded whitespace or characters which
       are special to COMMAND.COM and its ilk.  We don't quote all
       the arguments so the command line won't grow larger than
       the 126-char limit, if it doesn't have to.  */
    if (!__dosexec_in_system && strpbrk(argv[i], " \t<>|'\"%") != 0)
    {
      char *d = cmdline + strlen(cmdline);
      char *s = argv[i];
      /* COMMAND.COM doesn't understand escaped quotes, so we must
	 insert additional quotes around redirection characters if
	 it would seem to COMMAND.COM we're outside of quoted part.
	 This variable keeps track of whether we are in- or outside
	 quotes as far as COMMAND.COM is concerned.  */
      int  outside_quote = 0;

      *d++ = '"';
      while (*s)
      {
	if (*s == '"')
	{
	  outside_quote = !outside_quote;
	  *d++ = '\\';
	}
	else if (outside_quote && (*s == '|' || *s == '<' || *s == '>'))
	{
	  *d++ = '"';
	  *d++ = *s++;
	  *d++ = '"';
	  continue;
	}
	else if (*s == '%')
	  *d++ = '%';
	*d++ = *s++;
      }
      *d++ = '"';
      *d++ = '\0';
    }
    else
      strcat(cmdline, argv[i]);
  }
  for (i = 0; envp[i]; i++)
    if (strncmp(envp[i], "COMSPEC=", 8) == 0)
      comspec = envp[i] + 8;
  if (!comspec)
    for (i = 0; _environ[i]; i++)
      if (strncmp(_environ[i], "COMSPEC=", 8) == 0)
        comspec = _environ[i] + 8;
  if (!comspec)
    comspec = "c:\\command.com";

  if ((cmdline_len = strlen(cmdline)) > CMDLEN_LIMIT)
  {
    size_t cmdline_limit;

    cmdline_limit = _shell_cmdline_limit(comspec);
    if (cmdline_limit == 0)	/* unknown shell */
      cmdline_limit = CMDLEN_LIMIT;

    if (cmdline_len > cmdline_limit)
    {
      cmdline[CMDLEN_LIMIT] = '\0';
      errno = E2BIG;
    }
    else
    {
      int comspec_len = strlen(comspec);
      char *ptr;

      cmdline_var = (char *)alloca(__cmdline_str_len
				   + comspec_len + 3 + cmdline_len + 1);
      ptr = cmdline_var;

      /* Dump into CMDLINE the name of the shell to execute and the
         entire command line.  */
      strcpy(cmdline_var, __cmdline_str);
      ptr += __cmdline_str_len;

      if (memchr(comspec, ' ', comspec_len) != NULL)
      {
	*ptr++ = '"';
	memcpy(ptr, comspec, comspec_len);
	ptr += comspec_len;
	*ptr++ = '"';
      }
      else
      {
	memcpy(ptr, comspec, comspec_len);
	ptr += comspec_len;
      }

      *ptr++ = ' ';
      strcpy (ptr, cmdline);
    }
  }

  tbuf_beg = tbuf_ptr = __tb;
  tbuf_len = __tb_size;
  tbuf_end = tbuf_ptr + tbuf_len - 1;
  return direct_exec_tail(comspec, cmdline, envp, 0, 2, cmdline_var);
}

static int script_exec(const char *program, char **argv, char **envp)
{
  char line[130], interp[FILENAME_MAX], iargs[130];
  FILE *f;
  char **new_args;
  int i, has_args = 0;
  unsigned int ln;
  char *base, *p;
  int has_extension = 0, has_drive = 0;
  char pinterp[FILENAME_MAX];
  int (*spawnfunc)(int, const char *, char *const [], char *const []);
  int e = errno;

  f = fopen(program, "rt");
  if (!f)
  {
    errno = ENOENT;
    return -1;
  }
  fgets(line, sizeof(line), f);
  fclose(f);

  if (strncmp(line, "#!", 2))		/* prevent infinite loop */
    return go32_exec(program, argv, envp);

  /* Paranoia: is this at all a text file?  */
  for (ln = 0; ln < sizeof(line) - 1 && line[ln] != '\0'; ln++)
    if (line[ln] < 7 && line[ln] >= 0)
      return direct_exec(program, argv, envp);

  iargs[0] = 0;
  interp[0] = 0;
  sscanf(line, "#! %s %[^\n]", interp, iargs);

  /* If no interpreter, invoke the default shell in $COMSPEC.  */
  if (interp[0] == 0)
    return __dosexec_command_exec(program, argv, envp); /* it couldn't be .exe or .com if here */
  if (iargs[0])
    has_args = 1;

  for (i = 0; argv[i]; i++);
  new_args = (char **)alloca((i + 2 + has_args) * sizeof(char *));
  for (i = 0; argv[i]; i++)
    new_args[i + 1 + has_args] = unconst(argv[i], char *);
  new_args[i + 1 + has_args] = 0;
  /* Some interpreters might have their own ideas about $PATH.
     Therefore, pass them the full pathname of the script.  */
  new_args[0] = new_args[1 + has_args] = unconst(program, char *);
  if (has_args)
    new_args[1] = iargs;

  /* If INTERP is a Unix-style pathname, like "/bin/sh", we will try
     it with the usual extensions and, if that fails, will further
     search for the basename of the shell along the PATH; this
     allows to run Unix shell scripts without editing their first line.  */
  for (base = p = interp; *p; p++)
  {
    if (*p == '.')
      has_extension = 1;
    if (*p == '/' || *p == '\\' || *p == ':')
    {
      if (*p == ':')
	has_drive = 1;
      has_extension = 0;
      base = p + 1;
    }
  }

  if (has_drive || has_extension)
  {
    strcpy (pinterp, interp);
    spawnfunc = spawnvpe;
  }
  else if (__dosexec_find_on_path(interp, (char **)0, pinterp)
	   || __dosexec_find_on_path(base, envp, pinterp))
  {
    spawnfunc = spawnve;	/* no need to search on PATH: we've found it */
    errno = e;
  }
  else
    return -1;

  /* pinterp may contain backslashes because of __dosexec_find_on_path.
     Convert them to slashes so Unix shell scripts can run without editing.  */
  p = pinterp;
  while (*p)
  {
    if ((*p) == '\\')
      *p = '/';
    ++p;
  }

  i = (*spawnfunc)(P_WAIT, pinterp, new_args, envp);
  return i;
}

/* Note: the following list is not supposed to mention *every*
   possible extension of an executable file.  It only mentions
   those extensions that can be *omitted* when you invoke the
   executable from one of the shells used on MSDOS.  */

#define INTERP_FLAG_SKIP_SEARCH 1

static struct {
  const char *extension;
  int (*interp)(const char *, char **, char **);
  unsigned char flags;
} interpreters[] = {
  { ".com", direct_exec, 0 },
  { ".exe", go32_exec, 0 },
  { ".bat", __dosexec_command_exec, 0 },
  { ".btm", __dosexec_command_exec, 0 },
  { ".sh",  script_exec, INTERP_FLAG_SKIP_SEARCH },  /* Bash */
  { ".ksh", script_exec, INTERP_FLAG_SKIP_SEARCH },
  { ".pl",  script_exec, INTERP_FLAG_SKIP_SEARCH },  /* Perl */
  { ".sed", script_exec, INTERP_FLAG_SKIP_SEARCH },  /* Sed */
  { "",     go32_exec, 0 },
  { 0,      script_exec, 0 },  /* every extension not mentioned above calls it */
  { 0,      0, 0 },
};

/* This is the index into the above array of the interpreter
   which is called when the program filename has no extension.  */
#define INTERP_NO_EXT (sizeof(interpreters)/sizeof(interpreters[0]) - 3)

/*-------------------------------------------------*/

char *
__dosexec_find_on_path(const char *program, char *envp[], char *buf)
{
  char *pp, *rp, *pe;
  const char *ptr;
  int i, has_dot = 0, has_path = 0;
  int tried_dot = 0;
  int e = errno, blen = strlen(program);

  if (blen > FILENAME_MAX - 1)
  {
    errno = ENAMETOOLONG;
    return 0;
  }
  strlcpy(buf, program, blen + 1);
  rp = buf + blen;

  for (ptr=program; *ptr; ptr++)
  {
    if (*ptr == '.')
      has_dot = 1;
    if (*ptr == '/' || *ptr == '\\' || *ptr == ':')
    {
      has_path = 1;
      has_dot = 0;
    }
  }

  /* Under LFN, we must try the extensions even if PROGRAM already has one.  */
  if (!has_dot || _use_lfn(program))
    for (i = 0; interpreters[i].extension; i++)
    {
      if (interpreters[i].flags & INTERP_FLAG_SKIP_SEARCH)
        continue;
      strcpy(rp, interpreters[i].extension);
      if (access(buf, 0) == 0 && access(buf, D_OK))
      {
	/* If some of the `access' calls failed, `errno' will hold
	   the reason for the failure which is irrelevant to the
	   caller (we *did* find the executable).  Restore the value
	   `errno' had when we were called.  */
	errno = e;
	return buf;
      }
    }

  if (access(buf, 0) == 0 && access(buf, D_OK))
  {
    errno = e;
    return buf;
  }

  if (has_path || !envp)
    return 0;
  *rp = 0;

  pp = 0;
  for (i = 0; envp[i]; i++)
    if (strncmp(envp[i], "PATH=", 5) == 0)
      pp = envp[i] + 5;
  if (pp == 0)
    return 0;

  while (1)
  {
    if (!tried_dot)
    {
      rp = buf;
      pe = pp;
      tried_dot = 1;
    }
    else
    {
      rp = buf;
      for (pe = pp; *pe && *pe != ';'; pe++)
        *rp++ = *pe;
      pp = pe+1;
      if (rp > buf && rp[-1] != '/' && rp[-1] != '\\' && rp[-1] != ':')
        *rp++ = '/';
    }
    for (ptr = program; *ptr; ptr++)
      *rp++ = *ptr;
    *rp = 0;

    if (!has_dot || _use_lfn(buf))
      for (i = 0; interpreters[i].extension; i++)
      {
        strcpy(rp, interpreters[i].extension);
        if (access(buf, 0) == 0 && access(buf, D_OK))
	{
	  errno = e;
          return buf;
	}
      }
    if (access(buf, 0) == 0 && access(buf, D_OK))
    {
      errno = e;
      return buf;
    }
    if (*pe == 0)
      return 0;
  }
}

static int
find_interpreter (const char *path, char *ext)
{
  int i;
  int is_dir = 0;

  for (i = 0; interpreters[i].extension; ++i)
  {
    if (stricmp(ext, interpreters[i].extension) == 0)
      break;
  }

  if (access(path, F_OK) == 0 && (is_dir = access(path, D_OK)) != 0)
    return i;

  errno = is_dir ? EISDIR : ENOENT;
  return -1;
}

static int
find_extension (const char *path, char *ext)
{
  int i;
  int is_dir = 0;

  for (i = 0; interpreters[i].extension; ++i)
  {
    if (interpreters[i].flags & INTERP_FLAG_SKIP_SEARCH)
      continue;
    strcpy(ext, interpreters[i].extension);
    if (access(path, F_OK) == 0 && (is_dir = access(path, D_OK)) != 0)
      return i;
  }

  *ext = 0;
  errno = is_dir ? EISDIR : ENOENT;
  return -1;
}


int __spawnve(int mode, const char *path, char *const argv[],
              char *const envp[])
{
  return __djgpp_spawn(mode, path, argv, envp, SPAWN_EXTENSION_SRCH);
}

#ifdef __APPLE__
int spawnve(int mode, const char *path, char *const argv[],
              char *const envp[])
{
  return __spawnve(mode, path, argv, envp);
}
#else
int __attribute__((alias("__spawnve")))
spawnve(int mode, const char *path, char *const argv[],
              char *const envp[]);
#endif

int __djgpp_spawn(int mode, const char *path, char *const argv[],
                     char *const envp[], unsigned long flags)
{
  /* This is the one that does the work! */
  union { char *const *x; char **p; } u;
  const int no_interp_found = -1;
  int i = no_interp_found;
  char **argvp;
  char **envpp;
  char rpath[FILENAME_MAX + 4], *rp_end, *rp_ext = 0;
  int e = errno;
  int ret_code;

  if (path == 0 || argv[0] == 0)
  {
    errno = EINVAL;
    return -1;
  }
  if (mode == P_NOWAIT)
  {
    errno = ENOSYS;
    return -1;
  }
  if (strlen(path) > FILENAME_MAX - 1)
  {
    errno = ENAMETOOLONG;
    return -1;
  }

  u.x = argv; argvp = u.p;
  u.x = envp; envpp = u.p;

  /* Set defaults for the environment and search method.  */
  if (envpp == NULL)
    envpp = _environ;

  if (flags == 0)
    flags |= SPAWN_EXTENSION_SRCH;

  /* Copy the path to rpath and also mark where the extension is.  */
  fflush(stdout); /* just in case */
  for (rp_end = rpath; *path; *rp_end++ = *path++)
  {
    if (*path == '.')
      rp_ext = rp_end;
    if (*path == '\\' || *path == '/')
      rp_ext = 0;
  }
  *rp_end = 0;

  /* Perform an extension search when the flag SPAWN_NO_EXTENSION_SRCH is not
     present.  If LFN is supported on the volume where rpath resides, we
     might have something like foo.bar.exe or even foo.exe.com.
     If so, look for RPATH.ext before even trying RPATH itself.
     Otherwise, try to add an extension to a file without one.  */
  if (flags & SPAWN_EXTENSION_SRCH)
  {
    if (_use_lfn(path) || !rp_ext)
    {
      i = find_extension(rpath, rp_end);
      /* When LFN is supported and an extension search fails, the go32_exec
         interpreter will be selected instead of none.  In this case,
         set the interpreter to none so the interpreter will be selected
         from the existing extension.  */
      if ((i != no_interp_found) && rp_ext && *rp_end == 0)
        i = no_interp_found;
    }
  }

  /* If no interpreter has already been detected, find one based on the
     extension in rpath.  */
  if (i == no_interp_found)
    i = find_interpreter(rpath, rp_ext ? rp_ext : rp_end);

  /* The file does not exist. Return with errno set either by find_extension
     or find_interpreter to indicate the error.  */
  if (i == no_interp_found)
    return -1;

  /* If adding an extension makes the path longer than FILENAME_MAX,
     reject the path as too long.  */
  if (*rp_end && (rp_end - rpath + strlen(rp_end)) > FILENAME_MAX - 1)
  {
    errno = ENAMETOOLONG;
    return -1;
  }

  errno = e;
  ret_code = interpreters[i].interp(rpath, argvp, envpp);
  if (mode == P_OVERLAY)
    exit(ret_code);
  return ret_code;
}
