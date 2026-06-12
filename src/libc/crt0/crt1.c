/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2004 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <crt0.h>
#include <string.h>
#include <libc/internal.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <go32.h>
#include <stubinfo.h>
#include <dpmi.h>
#include <libc/farptrgs.h>
#include <pc.h>
#include <conio.h>
#include <libc/bss.h>
#include <fcntl.h>
#include <unistd.h>
#include <libc/environ.h>
#include <dos.h> /* for _osmajor/_osminor */
#include <libc/unconst.h>

/* Global variables */

#define ds _my_ds()

/* This gets incremented each time the program is started.
   Programs (such as Emacs) which dump their code to create
   a new executable, cause this to be larger than 2.  Library
   functions that cache info in static variables should check
   the value of `__bss_count' if they need to reinitialize
   the static storage.  */
int __bss_count = 1;

char **_environ;

int __crt0_argc;
char **__crt0_argv;

/* Local variables */

static void
setup_core_selector(void)
{
  int c = __dpmi_allocate_ldt_descriptors(1);
  if (c == -1)
  {
    /* We cannot continue without a valid _dos_ds selector, since
       setup_screens, called next, already uses it, and will crash
       unless we do something.

       The failure to allocate a descriptor is known to happen on
       Windows, when a long nested job (e.g., large recursive
       Makefile) is in progress, due to the fact that the Windows DPMI
       host leaks descriptors.

       So we want to print an error message and exit; but the tricky
       part is that we cannot use the transfer buffer or any other
       buffer in conventional memory, because we don't have a selector
       for DOS memory.  That's why we use function 02 of Int 21h.
       (The idea is due to Morten Welinder.)  */
    static const char no_dos_ds_msg[] =
      "Cannot allocate selector for conventional memory; exiting\r\n";
    const char *p = no_dos_ds_msg;
    __dpmi_regs r = {0};

    while (*p)
    {
      r.h.ah = 2;  /* in case DOS clobbers it */
      r.h.dl = *p & 0xff;
      __dpmi_int(0x21, &r);
      p++;
    }
    __exit(107);  /* the same error code stub.asm uses for no selectors */
  }
  _dos_ds = c;

  /* This was -1, but DPMI doesn't guarantee more than 1M + 64K */
  __dpmi_set_segment_limit(_dos_ds, 0x10ffff);
}

static void
setup_screens(void)
{
  if(_farpeekw(_dos_ds, 0xffff3) == 0xfd80)	/* NEC PC98 ? */
  {
    ScreenPrimary = ScreenSecondary = 0xa0000;
  }
  else if (_farpeekb(_dos_ds, 0x449) == 7)
  {
    ScreenPrimary = 0xb0000;
    ScreenSecondary = 0xb8000;
  }
  else
  {
    ScreenPrimary = 0xb8000;
    ScreenSecondary = 0xb0000;
  }
}

static void
setup_go32_info_block(void)
{
  __dpmi_version_ret ver;

  __dpmi_get_version(&ver);

  _go32_info_block.size_of_this_structure_in_bytes = sizeof(_go32_info_block);
  __tb = _stubinfo->ds_segment * 16;
  __tb_size = _stubinfo->minkeep;
  _go32_info_block.pid = _stubinfo->psp_selector;
  _go32_info_block.master_interrupt_controller_base = ver.master_pic;
  _go32_info_block.slave_interrupt_controller_base = ver.slave_pic;
  _go32_info_block.linear_address_of_stub_info_structure = 0xffffffffU; /* force error */
  __dpmi_get_segment_base_address(_stubinfo->psp_selector,
    &_go32_info_block.linear_address_of_original_psp);
  _go32_info_block.run_mode = _GO32_RUN_MODE_DPMI;
  _go32_info_block.run_mode_info = (ver.major << 8) | (ver.minor);
}

char *__dos_argv0;

static void
setup_environment(void)
{
  char *dos_environ, *cp;
  short env_selector;
  int env_count=0;

  dos_environ = (char *)alloca(_stubinfo->env_size);
  fmemcpy2(&env_selector, DP(_stubinfo->psp_selector, 0x2c), 2);
  fmemcpy2(dos_environ, DP(env_selector, 0), _stubinfo->env_size);
  cp = dos_environ;
  while (*cp) { /* repeat until two NULs */
    env_count++;
    while (*cp) cp++; /* skip to NUL */
    cp++; /* skip to next character */
  }
  _environ = (char **)malloc((env_count+1) * sizeof(char *));
  if (_environ == 0)
    return;

  cp = dos_environ;
  env_count = 0;
  while (*cp) { /* repeat until two NULs */
    /* putenv assumes each string is malloc'd */
    _environ[env_count] = (char *)malloc(strlen(cp)+1);
    strcpy(_environ[env_count], cp);
    env_count++;
    while (*cp) cp++; /* skip to NUL */
    cp++; /* skip to next character */
  }
  _environ[env_count] = 0;

  /*
   * Call putenv so that its static counters are computed. If this
   * is not done, programs that manipulate `_environ' directly will crash,
   * when `DJGPP' is not set in the environment.
   */
  putenv(unconst("", char *));

  __dos_argv0 = (char *)malloc(strlen(cp + 3)+1);
  if (__dos_argv0 == 0)
    abort();
  strcpy(__dos_argv0, cp+3);
}

static char *prog_name;

static void
setup_pname(void)
{
  char *ap, *fc;

  fc = __dos_argv0;
  for(ap = __dos_argv0; *ap; ap++)
    if(*ap == ':' || *ap == '\\' || *ap == '/')
      fc = ap + 1;

  if(_stubinfo->argv0[0])
    {
      fc = _stubinfo->argv0;
    }

  prog_name = (char *)calloc(1, strlen(fc) + 1);
  strcpy(prog_name, fc);
}

extern void __setup_file_rec_list(void);

char __PROXY[] = " !proxy";
size_t __PROXY_LEN = sizeof(__PROXY)-1;
unsigned short _os_trueversion;

static void
setup_os_version(void)
{
  unsigned short v;
  _os_trueversion = _get_dos_version(1);
  v = _get_dos_version(0); /* Get the reported version */
  _osmajor = (v >> 8) & 0xff; /* paranoia */
  _osminor = v & 0xff;
}


void
__crt1_startup(main_t *main)
{
  __bss_count ++;
  __crt0_argv = 0;
  setup_os_version();
  setup_core_selector();
  setup_screens();
  setup_go32_info_block();
  __djgpp_exception_setup();
  init_sys_siglist();
  init_confstr();
  setup_environment();
  __environ_changed++;
  __setup_file_rec_list();
  /* Make so rest of startup could use LFN.  */
  (void)_USE_LFN;
  setup_pname();
  __crt0_load_environment_file(prog_name);
  (void)_USE_LFN;	/* DJGPP.ENV might have set $LFN */
  free(prog_name);
  __crt0_setup_arguments();
  _npxsetup(__crt0_argv ? __crt0_argv[0] : __dos_argv0);
  _crt0_init_mcount();
  gppconio_init();
  errno = 0;	/* ANSI says errno should be zero at program startup */
  exit(main(__crt0_argc, __crt0_argv, _environ));
}

#if !USE64
extern int main(int, char **, char **);
void
___crt1_startup(void)
{
  __crt1_startup(main);
}
#endif
