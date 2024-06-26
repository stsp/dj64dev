/* Copyright (C) 2012 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_process_h_
#define __dj_include_process_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

int _dos_exec(const char *program, const char *args, char *const _envp[], const char *_lcmd);

int execl(const char *_path, const char *_argv0, ...);
int execle(const char *_path, const char *_argv0, ... /*, char *const _envp[] */);
int execlp(const char *_path, const char *_argv0, ...);
int execlpe(const char *_path, const char *_argv0, ... /*, char *const _envp[] */);

int execv(const char *_path, char *const _argv[]);
int execve(const char *_path, char *const _argv[], char *const _envp[]);
int execvp(const char *_path, char *const _argv[]);
int execvpe(const char *_path, char *const _argv[], char *const _envp[]);

int spawnl(int _mode, const char *_path, const char *_argv0, ...);
int spawnle(int _mode, const char *_path, const char *_argv0, ... /*, char *const _envp[] */);
int spawnlp(int _mode, const char *_path, const char *_argv0, ...);
int spawnlpe(int _mode, const char *_path, const char *_argv0, ... /*, char *const _envp[] */);

int spawnv(int _mode, const char *_path, char *const _argv[]);
int spawnve(int _mode, const char *_path, char *const _argv[], char *const _envp[]);
int spawnvp(int _mode, const char *_path, char *const _argv[]);
int spawnvpe(int _mode, const char *_path, char *const _argv[], char *const _envp[]);

int __djgpp_spawn(int _mode, const char *_path, char *const _argv[], char *const _envp[], unsigned long _flags);

#define P_WAIT		1
#define P_NOWAIT	2	/* always generates error */
#define P_OVERLAY	3

#define SPAWN_EXTENSION_SRCH    1
#define SPAWN_NO_EXTENSION_SRCH 2

#define __spawn_leak_workaround             0x0001 /* free descriptor leaks */

extern int __spawn_flags;

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_process_h_ */
