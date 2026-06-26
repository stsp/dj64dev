/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <sys/system.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <process.h>
#include <dos.h>
#include <libc/bss.h>
#include <libc/dosexec.h>
#include <libc/unconst.h>
#include <libc/file.h> /* for fileno() */

extern char **_environ;

typedef enum {
  REDIR_INPUT,
  REDIR_OUTPUT,
  REDIR_APPEND,
  PIPE,
  SEMICOLON,
  EOL,
  WORD,
  UNMATCHED_QUOTE
} cmd_sym_t;

static cmd_sym_t get_sym (char *s, char **beg, char **end);
static char *    __unquote (char *to, const char *beg, const char *end);

int __system_flags = __system_redirect
		   | __system_handle_null_commands
		   | __system_use_shell
		   | __system_allow_long_cmds;

static int sys_flags;

static inline int
emiterror (const char *s, int err)
{
  write (2, s, strlen (s));
  if (err > 0)
  {
    char *msg = strerror (err);
    size_t msg_len = strlen (msg);

    write (2, ": ", 2);
    write (2, msg, msg_len);
  }
  write (2, "\r\n", 2);	/* '\r' in case they put it into binary mode */
  fsync (2);		/* in case they've redirected stderr */
  return -1;
}

static char command_com[] = "c:\\command.com";

/* Call the system shell $(SHELL) or $(COMSPEC) with the
   command line "PROG CMDLINE".  */
int
_shell_command (const char *prog, const char *cmdline)
{
  char *comspec = getenv ("COMSPEC");
  char *shell = 0;

  if (!prog)
    prog = "";
  if (!cmdline)
    cmdline = "";

  if (sys_flags & __system_use_shell)
    shell = getenv ("SHELL");
  if (!shell)
    shell = comspec;
  /* Is it worth the hassle to get the boot drive (Int 21h/AX=3305h)
     and look for COMMAND.COM there if COMSPEC fails?  */
  if (!shell)
    shell = command_com;

  if (!*prog && !*cmdline)
  {
    /* Special case: zero or empty command line means just invoke
       the command interpreter and let the user type ``exit''.  */
    return _dos_exec (shell, "", _environ, 0);
  }
  else if (_is_dos_shell (shell))
  {
    int cmd_tail_alloc = 3 + strlen(prog) + 1 + strlen (cmdline) + 1;
    char *cmd_tail = (char *)alloca(cmd_tail_alloc);
    const char *s = prog;
    char *d = cmd_tail + 3;
    size_t cmd_tail_len;

    strcpy (cmd_tail, "/c ");
    while ((*d = *s++) != 0)
    {
      if (*d == '/')
	*d = '\\';
      d++;
    }

    if (*cmdline)
    {
      if (*prog)
	*d++ = ' ';

      strcpy (d, cmdline);
    }

    /* [4N]DOS.COM can support upto 255 chars per command line.
       Windows PE-COFF executables support long command lines
       passed through the CMDLINE environment variable, which
       can handle up to 1024 characters.  */
    if ((cmd_tail_len = strlen (cmd_tail)) > 126)
    {
      size_t cmd_len_limit;
      size_t shell_len = strlen (shell);

      cmd_len_limit = _shell_cmdline_limit (shell);
      if (cmd_len_limit == 0) /* unknown shell */
	cmd_len_limit = 126;

      if (cmd_tail_len > cmd_len_limit)
      {
        errno = E2BIG;
        return emiterror ("Command line too long.", 0);
      }
      else
      {
	extern char __cmdline_str[];
	extern size_t __cmdline_str_len;
	char *cmdline_var = (char *)alloca (shell_len + 3 + cmd_tail_len
					    + __cmdline_str_len);
	char *ptr = cmdline_var;

	/* Dump into CMDLINE the shell to execute and the entire
	   command line.  */
	strcpy (cmdline_var, __cmdline_str);
	ptr += __cmdline_str_len;
	if (memchr (shell, ' ', shell_len))
	{
	  *ptr++ = '"';
	  memcpy (ptr, shell, shell_len);
	  ptr += shell_len;
	  *ptr++ = '"';
	}
	else
	{
	  memcpy (ptr, shell, shell_len);
	  ptr += shell_len;
	}

	*ptr++ = ' ';
	strcpy (ptr, cmd_tail);

	if (strlen (cmdline_var) > cmd_len_limit)
	{
	  errno = E2BIG;
	  return emiterror ("Command line too long.", 0);
	}
	else
	  return _dos_exec(shell, cmd_tail, _environ, cmdline_var);
      }
    }
    else
      return _dos_exec (shell, cmd_tail, _environ, 0);
  }
  else
  {
    /* Assume this is a `sh' work-alike which can be invoked like
       this:
       		sh file

       where `file' holds the entire command line.

       (Another possibility is a response file, but that breaks the
       ``here document'' feature of the shell.)  */
    FILE *respf;
    int   e = errno;
    char *atfile = (char *) alloca (L_tmpnam);
    char *cmd_tail = (char *)alloca (L_tmpnam + 1);

    errno = 0;
    respf = fopen (tmpnam (atfile), "wb");

    if (respf)
    {
      int retval;

      errno = e;
      if (*prog)
      {
	fputs (prog, respf);
	fputc (' ', respf);
      }
      fputs (cmdline, respf);
      fputc ('\n', respf);
      fclose (respf);
      strcpy (cmd_tail, " ");
      strcat (cmd_tail, atfile);
      retval = _dos_exec (shell, cmd_tail, _environ, 0);
      remove (atfile);
      return retval;
    }
    else
      return emiterror ("Cannot open script file for $SHELL", errno);
  }
}

/* If this function finds PROG on the PATH, it breaks the command
   tail into words and calls `spawnve' to invoke PROG with the list
   of words as arguments.
   Otherwise, it calls the shell, on the assumption that it's a
   built-in command, or alias, or something.

   (We cannot just pass the command tail as a single argument,
   because this will look to the child as if the entire command line
   was a single quoted argument.  In particular, it will effectively
   disable file wildcards expansion by the child.)  */

static int
plainsystem(const char *prog, char *args)
{
  char found_at[FILENAME_MAX];
  int e = errno;

  if (__dosexec_find_on_path (prog, _environ, found_at))
  {
    char **pargv, **this_arg;
    int    pargc = 2;		/* PROG is one, terminating 0 is another */
    char *pcmd = args;
    char *b, *e2;

    if (! (sys_flags & __system_allow_long_cmds))
    {
      if (strlen (args) > 126)
      {
	errno = E2BIG;
	return emiterror ("Command line too long.", 0);
      }

      return _dos_exec (found_at, args, _environ, 0);
    }

    /* Pass 1: how many arguments do we have?  */
    while (*pcmd)
    {
      /* Only words and the terminating 0 are valid at this point.  */
      if (get_sym (pcmd, &b, &e2) == WORD)
	pargc++;
      else if (*b)
      {
	errno = EINVAL;
	return emiterror ("Syntax error.", 0);
      }

      pcmd = e2;
    }

    /* Pass 2: put program, the arguments and the terminating 0.  */
    this_arg = pargv = (char **)alloca (pargc * sizeof (char *));
    *this_arg++ = strcpy ((char *)alloca (strlen (prog) + 1), prog);
    --pargc;

    for (pcmd = args; --pargc; pcmd = e2, this_arg++)
    {
      get_sym (pcmd, &b, &e2);
      *this_arg = (char *)alloca (e2 - b + 1);
      strncpy (*this_arg, b, e2 - b);
      (*this_arg)[e2 - b] = '\0';
    }
    *this_arg = 0;

    return __spawnve(P_WAIT, found_at, pargv, (char * const *)_environ);
  }
  else
  {
    /* PROG is nowhere on the PATH.  If it got explicit ".exe",
       ".bat", ".btm" or ".com" extension, return an error.  */
    const char *endcmd = 0;
    int i;

    for (i = 0; prog[i]; i++)
    {
      if (prog[i] == '.')
	endcmd = prog+i+1;
      if (prog[i] == '\\' || prog[i] == '/')
	endcmd = 0;
    }
    if (endcmd
	&& (stricmp (endcmd, "exe") == 0
	    || stricmp (endcmd, "com") == 0
	    || stricmp (endcmd, "bat") == 0
	    || stricmp (endcmd, "btm") == 0))
    {
      errno = ENOENT;
      return -1;
    }
    else
    {
      errno = e;  /* don't return errno from failed $PATH search */
      /* Last resort: try the shell.  */
      return _shell_command (prog, args);
    }
  }
}


/* ------------------------------------------------------------------------ */
/* Now follows the overlay that handles redirection.  There should be no    */
/* fixed limits in this code.                                               */

/* Check if two strings compare equal upto given length.  */

#define CHECK_FOR(s, c)		check_for(s, c, sizeof(c) - 1)

static int
check_for(const char *s, const char *cmd, int cmdl)
{
  while (cmdl)
  {
    if (*s++ != *cmd++)
      return 0;
    cmdl--;
  }

  if (isgraph ((unsigned char)*s))
    return 0;
  return 1;
}

/* This function handles certain dummy or simple commands and
   passes the rest to plainsystem.  */
static int
system1 (const char *cmd, char *cmdline)
{
  /* There are certain commands that are no-ops only with arguments,
     or only without them, or both.  There are other commands which
     we prefer to emulate (when the emulation is better than the
     original), but only if we're allowed to.

     Note that this function does the wrong thing if the command
     line is intended for a *real* shell; the assumption is that
     for these you just set `__system_call_cmdproc' and
     `__system_use_shell' in `__system_flags' and $SHELL in the
     environment, and let the shell do the job.  This function is
     meant to improve on COMMAND.COM and its ilk.

     The first character in CMDLINE cannot be a whitespace (it was
     removed by `system'), testing for non-empty arguments relies
     on this here.  */
  if ((sys_flags & __system_handle_null_commands)
      && (CHECK_FOR (cmd, "rem")
	  || CHECK_FOR (cmd, "exit")
	  || CHECK_FOR (cmd, "goto")
	  || CHECK_FOR (cmd, "shift")
	  || ((CHECK_FOR (cmd, "set")
	      || CHECK_FOR (cmd, "path")
	      || CHECK_FOR (cmd, "prompt"))  && *cmdline)))
    return 0;
  else if (CHECK_FOR (cmd, "chdir") || CHECK_FOR (cmd, "cd"))
  {
    if (*cmdline && (sys_flags & __system_ignore_chdir))
      return 0;
    else if (sys_flags & __system_emulate_chdir)
    {
      /* We can `chdir' better, because we know about forward
	 slashes, and also change the drive.  */
      if (*cmdline)
	return __chdir (cmdline);
      else
      {
	/* COMMAND.COM prints the current directory if given
	   no argument to CD.  */
	char wd[FILENAME_MAX];

	printf ("%s\n", __getcwd(wd, FILENAME_MAX-1)
		? wd : "Current drive is no longer valid");
	fflush (stdout);	/* make sure it's delivered */
	fsync (fileno (stdout));
	return 0;
      }
    }
  }
#if 0
  else if ((sys_flags & __system_emulate_echo)
	   && CHECK_FOR (cmd, "echo"))
    return do_echo (cmdline);
#endif
  else if (*cmd == 0)
    return emiterror ("Invalid null command.", 0);

  return plainsystem (cmd, cmdline);
}

/* Return a copy of a word between BEG and (excluding) END with all
   quoting characters removed from it.  */

static char *
__unquote (char *to, const char *beg, const char *end)
{
  const char *s = beg;
  char *d = to;
  int quote = 0;

  while (s < end)
  {
    switch (*s)
    {
      case '"':
      case '\'':
	if (!quote)
	  quote = *s;
	else if (quote == *s)
	  quote = 0;
	s++;
	break;
      case '\\':
	if (s[1] == '"' || s[1] == '\''
	    || (s[1] == ';'
		&& (__system_flags & __system_allow_multiple_cmds)))
	  s++;
	/* Fall-through.  */
      default:
	*d++ = *s++;
	break;
    }
  }

  *d = 0;
  return to;
}

/* A poor-man's lexical analyzer for simplified command processing.

   It only knows about these:

     redirection and pipe symbols
     semi-colon `;' (that possibly ends a command)
     argument quoting rules with quotes and `\'
     whitespace delimiters of words (except in quoted args)

   Returns the type of next symbol and pointers to its first and (one
   after) the last characters.

   Only `get_sym' and `unquote' should know about quoting rules.  */

static cmd_sym_t
get_sym (char *s, char **beg, char **end)
{
  int in_a_word = 0;

  while (isspace ((unsigned char)*s))
    s++;

  *beg = s;

  do {
    *end = s + 1;

    if (in_a_word
	&& (!*s || strchr ("<>| \t\n", *s)
	    || ((sys_flags & __system_allow_multiple_cmds) && *s == ';')))
    {
      --*end;
      return WORD;
    }

    switch (*s)
    {
      case '<':
	return REDIR_INPUT;
      case '>':
	if (**end == '>')
	{
	  ++*end;
	  return REDIR_APPEND;
	}
	return REDIR_OUTPUT;
      case '|':
	return PIPE;
      case ';':
	if (sys_flags & __system_allow_multiple_cmds)
	  return SEMICOLON;
	else
	  in_a_word = 1;
	break;
      case '\0':
	--*end;
	return EOL;
      case '\\':
	if (s[1] == '"' || s[1] == '\''
	    || (s[1] == ';' && (sys_flags & __system_allow_multiple_cmds)))
	  s++;
	in_a_word = 1;
	break;
      case '\'':
      case '"':
	{
	  char quote = *s++;

	  while (*s && *s != quote)
	  {
	    if (*s++ == '\\' && (*s == '"' || *s == '\''))
	      s++;
	  }
	  *end = s;
	  if (!*s)
	    return UNMATCHED_QUOTE;
	  in_a_word = 1;
	  break;
	}
      default:
	in_a_word = 1;
	break;
    }

    s++;

  } while (1);
}

/* This function handles redirection and passes the rest to system1
   or to the shell.  */
int
system (const char *cmdline)
{
  /* Set the feature bits for this run: either from the
     environment or from global variable.  */
  const char *envflags = getenv ("DJSYSFLAGS");
  const char *comspec  = getenv ("COMSPEC");
  const char *shell    = 0;
  int call_shell;

  sys_flags = __system_flags;
  if (envflags && *envflags)
  {
    char *stop;
    long flags = strtol (envflags, &stop, 0);

    if (*stop == '\0')
      sys_flags = flags;
  }

  if (sys_flags & __system_use_shell)
    shell = getenv ("SHELL");
  if (!shell)
    shell = comspec;
  if (!shell)
    shell = command_com;

  call_shell =
    (sys_flags & __system_call_cmdproc)
    || (!(sys_flags & __system_emulate_command) && _is_unixy_shell (shell));

  /* Special case: NULL means return non-zero if the command
     interpreter is available.  This is ANSI C requirement.

     If we will call the shell to do everything, we need to see
     whether it exists.  But if most of the work will be done by us
     anyway (like it usually is with stock DOS shell), return non-zero
     without checking, since our emulation is always ``available''.  */
  if (cmdline == 0)
  {
    if (!call_shell)
      return 1;
    else
    {
      char full_path[FILENAME_MAX];

      return __dosexec_find_on_path (shell, (char **)0, full_path) ? 1 : 0;
    }
  }

  /* Strip initial spaces (so that if the command is empty, we
     know it right here).  */
  while (isspace((unsigned char)*cmdline))
    cmdline++;

  /* Call the shell if:

     	the command line is empty
     or
	they want to always do it via command processor
     or
	$SHELL or $COMSPEC point to a unixy shell  */
  if (!*cmdline || call_shell)
    return _shell_command ("", cmdline);
  else
  {
    char *f_in = 0, *f_out = 0;
    int rm_in = 0, rm_out = 0;
    /* Assigned to silence -Wall */
    int h_in = 0, h_inbak = 0, h_out = 0, h_outbak = 0;
    char *s, *t, *u, *v, *tmp, *cmdstart;
    int result = 0, done = 0;
    int append_out = 0;
    cmd_sym_t token;
    char *prog;

    tmp = (char *)alloca(L_tmpnam);
    prog = (char *)alloca (L_tmpnam);

    s = strcpy ((char *)alloca (strlen (cmdline) + 1), cmdline);
    while (!done && result >= 0)
    {
      char **fp = &f_in;
      /* Assignements pacify -Wall */
      int hin_err = 0, hbak_err = 0, hout_err = 0;
      int needcmd = 1;
      int again;

      if (rm_in && f_in)
	remove (f_in);
      f_in = f_out;			/* Piping.  */
      rm_in = rm_out;
      f_out = 0;
      rm_out = 0;
      append_out = 0;

      cmdstart = s;

      do {
	again = 0;
	token = get_sym (s, &t, &u);	/* get next symbol */

	/* Weed out extra whitespace, leaving only a single
	   whitespace character between any two tokens.
	   This way, if we eventually pass the command to a
	   shell, it won't fail due to length > 126 chars
	   unless it really *is* that long.  */

	if (s == cmdstart)
	  v = s;	/* don't need blank at beginning of cmdline  */
	else
	  v = s + 1;
	if (t > v)
	{
	  strcpy (v, t);
	  u -= t - v;
	  t = v;
	}

	switch (token)
	{
	case WORD:
	  /* First word we see is the program to run.  */
	  if (needcmd)
	  {
	    __unquote (prog, t, u); /* unquote and copy to prog */
	    /* We can't grok commands in parentheses, so assume they
	       use a shell that knows about these, like 4DOS or `sh'.

	       FIXME: if the parenthesized group is NOT the first command
	       in a pipe, the commands that preceed it will be run twice.  */
	    if (prog[0] == '(')
	      return _shell_command ("", cmdline);
	    strcpy (s, u);	  /* remove program name from cmdline */
	    needcmd = 0;
	  }
	  else
	    s = u;
	  again = 1;
	  break;
	case REDIR_INPUT:
	case REDIR_OUTPUT:
	case REDIR_APPEND:
	  if (!(sys_flags & __system_redirect))
	    return _shell_command ("", cmdline);
	  if (token == REDIR_INPUT)
	  {
	    if (f_in)
	    {
	      result = emiterror ("Ambiguous input redirect.", 0);
	      errno = EINVAL;
	      goto leave;
	    }
	    fp = &f_in;
	  }
	  else if (token == REDIR_OUTPUT || token == REDIR_APPEND)
	  {
	    if (f_out)
	    {
	      result = emiterror ("Ambiguous output redirect.", 0);
	      errno = EINVAL;
	      goto leave;
	    }
	    fp = &f_out;
	    if (token == REDIR_APPEND)
	      append_out = 1;
	  }
	  if (get_sym (u, &u, &v) != WORD)
	  {
	    result = emiterror ("Target of redirect is not a filename.", 0);
	    errno = EINVAL;
	    goto leave;
	  }
	  /* The target of redirection might be quoted, so we need to
	     unquote it.  */
	  *fp = __unquote ((char *)alloca (v - u + 1), u, v);
	  strcpy (t, v);
	  again = 1;
	  break;
	case PIPE:
	  if (!(sys_flags & __system_redirect))
	    return _shell_command ("", cmdline);
	  if (f_out)
	  {
	    result = emiterror ("Ambiguous output redirect.", 0);
	    errno = EINVAL;
	    goto leave;
	  }

	  /* tmpnam guarantees unique names */
	  tmpnam(tmp);
	  f_out = strcpy ((char *)alloca (L_tmpnam), tmp);
	  rm_out = 1;
	  /* Fall through.  */
	case SEMICOLON:
	case EOL:
	  if (needcmd)
	  {
	    result = emiterror ("No command name seen.", 0);
	    errno = EINVAL;
	    goto leave;
	  }

	  /* Remove extra whitespace at end of command.  */
	  while (s > cmdstart && isspace ((unsigned char)s[-1])) s--;
	  while (t > s && isspace ((unsigned char)t[-1])) t--;
	  *t = 0;

#ifdef TEST
	  fprintf (stderr, "Input from: %s\nOutput%s to:  %s\n",
		   f_in ? f_in : "<stdin>",
		   append_out ? " appended" : "",
		   f_out ? f_out : "<stdout>");
	  fflush (stderr);
#endif
	  if (f_in)
	  {
	    int e = errno;

	    errno = 0;
	    h_in = open (f_in, O_RDONLY | O_BINARY);
	    hin_err = errno;
	    errno = 0;
	    h_inbak = dup (0);
	    hbak_err = errno;
	    dup2 (h_in, 0);
	    errno = e;
	  }
	  if (f_out)
	  {
	    int e = errno;

	    errno = 0;
	    h_out = open (f_out,
			  O_WRONLY | O_BINARY | O_CREAT
			  | (append_out ? O_APPEND: O_TRUNC),
			  S_IREAD | S_IWRITE);
	    hout_err = errno;
	    errno = 0;
	    h_outbak = dup (1);
	    hbak_err = errno;
	    fflush(stdout);  /* so any buffered chars will be written out */
	    dup2 (h_out, 1);
	    errno = e;
	  }
	  if (f_in && h_in < 0)
	    result = emiterror ("Cannot redirect input", hin_err);
	  else if ((f_in && h_inbak < 0) || (f_out && h_outbak < 0))
	    result = emiterror ("Out of file handles in redirect", hbak_err);
	  else if (f_out && h_out < 0)
	    result = emiterror ("Cannot redirect output", hout_err);

	  if (!result)
	  {
#ifdef TEST
	    fprintf (stderr, "system1 (\"%s\", \"%s\")\n", prog, cmdstart);
	    fflush (stderr);
#endif
	    /* Tell `__spawnve' it was invoked by `system', so it would
	       know how to deal with command-line arguments' quoting.  */
	    __dosexec_in_system = 1;
	    result = system1 (prog, cmdstart);
	    __dosexec_in_system = 0;
	  }
	  if (f_in)
	  {
	    dup2 (h_inbak, 0);
	    close (h_in);
	    close (h_inbak);
	  }
	  if (f_out)
	  {
	    dup2 (h_outbak, 1);
	    close (h_out);
	    close (h_outbak);
	  }

	  if (token == EOL)
	    done = 1;
	  else
	  {
	    if (token == SEMICOLON)
	    {
	      if (f_in && rm_in)
		remove (f_in);
	      if (f_out && rm_out)
		remove (f_out);
	      f_in = f_out = 0;
	    }
	    s = u;
	  }
	  break;
	case UNMATCHED_QUOTE:
	  result = emiterror ("Unmatched quote character.", 0);
	  errno = EINVAL;
	  goto leave;
	default:
	  result = emiterror ("I cannot grok this.", 0);
	  errno = EINVAL;
	  goto leave;
	}
      } while (again);
    }
  leave:
    if (rm_in && f_in)
      remove (f_in);
    if (rm_out && f_out)
      remove (f_out);

    return result;
  }
}

#ifdef TEST

#include <signal.h>

int main (int argc, char *argv[])
{
  __system_flags |= (__system_allow_multiple_cmds | __system_emulate_chdir);
  signal (SIGINT, SIG_IGN);
  if (argc > 1)
    {
      int i;
      printf ("system (\"%s\") gives this:\n\n", argv[1]);
      printf ("\n\nsystem() returned %d", (errno = 0, i = system (argv[1])));
      fflush (stdout);
      if (errno)
	perror ("");
      else
	puts ("");
      return 0;
    }
  else
    {
      int i;
      printf ("if system (NULL) returns non-zero, a shell is available:");
      printf ("\n\nsystem() returned %d", (errno = 0, i = system ((char *)0)));
      fflush (stdout);
      if (errno)
	perror ("");
      else
	puts ("");
      return 1;
    }
}

#endif
