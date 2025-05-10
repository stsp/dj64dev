/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <io.h>

static char msg[] = "Abort!\r\n";

void
abort(void)
{
  _write(STDERR_FILENO, msg, sizeof(msg)-1);
  raise(SIGABRT);	/* this will generate traceback and won't return */
  _exit(1);
}

#ifdef TEST

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <process.h>
#include <sys/exceptn.h>

int main (int argc, char *argv[])
{
  int status = 0;

  errno = 0;
  if (argc > 1)
    {
      if (strcmp (argv[1], "abort") == 0)
	abort ();
      else if (strcmp (argv[1], "toggle") == 0)
	{
	  __djgpp_exception_toggle ();
	  abort ();
	}
    }
  else
    {
      fprintf (stderr, "\tType `%s abort RET'\n"
		       "or\n"
		       "\t     `%s toggle RET'\n", argv[0], argv[0]);
      status = system ("");
    }

  fprintf (stderr, "Child returned %d\n", status);
  if (errno)
    perror ("spawn");
  return 0;
}
#endif
