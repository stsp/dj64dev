/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef P_tmpdir
#define P_tmpdir "c:/"
#endif

static const char *tdirs[] = {"TMPDIR", "TEMP", "TMP", 0};
static const char x8[] = "XXXXXXXX";

char *
tempnam(const char *tmpdir, const char *pfx)
{
  char _template[FILENAME_MAX];
  size_t lastc;
  char *s, *fname;
  const char **p = tdirs;

  /* Supply a default directory, if they didn't.  */
  while (!tmpdir || !*tmpdir || access (tmpdir, D_OK))
    {
      if (!*p)
	{
	  tmpdir = P_tmpdir;
	  if (access (tmpdir, D_OK))
	    return (char *)0;	/* can this ever happen? */
	  break;
	}
      tmpdir = getenv (*p++);
    }

  /* Append a slash, if needed.  */
  lastc = strlen (strcpy (_template, tmpdir)) - 1;
  if (_template[lastc] != '/' && _template[lastc] != '\\')
    _template[++lastc] = '/';

  /* Append the prefix.  */
  if (!pfx || !*pfx)
    pfx = "tm";
  strcpy (_template + lastc + 1, pfx);

  /* Create the template.  */
  strlcat (_template, x8, sizeof(_template));

  s = mktemp (_template);
  if (s)
    {
      fname = (char *)malloc (strlen (s) + 1);

      if (fname)
	return strcpy (fname, s);
    }
  return (char *)0;
}

#ifdef TEST

#include <conio.h>

int main (void)
{
  cprintf ("tempnam (\"c:/tmp/\", \"foo\") -> \"%s\"\r\n",
	  tempnam ("c:/tmp/", "foo")); getch();
  cprintf ("tempnam (\"c:/tmp\", \"foo\") -> \"%s\"\r\n",
	  tempnam ("c:/tmp", "foo")); getch();
  cprintf ("tempnam (\"c:/tmp\", \"zoobarically-long\") -> \"%s\"\r\n",
	  tempnam ("c:/tmp", "zoobarically-long")); getch();
  cprintf ("tempnam (\"c:/tmp/\", NULL) -> \"%s\"\r\n",
	  tempnam ("c:/tmp/", 0)); getch();
  cprintf ("tempnam (\"c:/tmp/\", \"\") -> \"%s\"\r\n",
	  tempnam ("c:/tmp/", "")); getch();
  cprintf ("tempnam (\"x:/no:such:dir/\", \"foo\") -> \"%s\"\r\n",
	  tempnam ("x:/no:such:dir/", "foo")); getch();
  cprintf ("tempnam (\"\", \"foo\") -> \"%s\"\r\n",
	  tempnam ("", "foo")); getch();
  cprintf ("tempnam (NULL, \"foo\") -> \"%s\"\r\n",
	  tempnam (0, "foo")); getch();
  putenv ("TMPDIR="); cprintf ("no TMPDIR: ");
  cprintf ("tempnam (\"x:/no:such:dir/\", \"foo\") -> \"%s\"\r\n",
	  tempnam ("x:/no:such:dir/", "foo")); getch();
  putenv ("TEMP="); cprintf ("no TMPDIR, no TEMP: ");
  cprintf ("tempnam (\"x:/no:such:dir/\", \"foo\") -> \"%s\"\r\n",
	  tempnam ("x:/no:such:dir/", "foo")); getch();
  putenv ("TMP="); cprintf ("no TMPDIR, no TEMP, no TMP: ");
  cprintf ("tempnam (\"x:/no:such:dir/\", \"foo\") -> \"%s\"\r\n",
	  tempnam ("x:/no:such:dir/", "foo")); getch();
  putenv ("TEMP=."); cprintf ("TEMP=.: ");
  cprintf ("tempnam (\"x:/no:such:dir/\", \"foo\") -> \"%s\"\r\n",
	  tempnam ("x:/no:such:dir/", "foo")); getch();
  return 0;
}

#endif
