/*
 *  libsocket - BSD socket like library for DJGPP
 *  Copyright 1997, 1998 by Indrek Mandre
 *  Copyright 1997-2000 by Richard Dawe
 *
 *  Portions of libsocket Copyright 1985-1993 Regents of the University of
 *  California.
 *  Portions of libsocket Copyright 1991, 1992 Free Software Foundation, Inc.
 *  Portions of libsocket Copyright 1997, 1998 by the Regdos Group.
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * ini.c - Windows .INI file parsing functions
 *
 * This file was written by Richard Dawe.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lsck/ini.h>

/* ---------------------------
 * - GetPrivateProfileString -
 * --------------------------- */

/* This does the same as the Windows API call of the same name. */

int GetPrivateProfileString (const char *appname, const char *keyname, const char *deflt,
			     char *dest, size_t destsize, char *filename)
{
    FILE *fp;
    char buf[256], *line = NULL, *p = NULL, *q = NULL;
    int in_section = 0, got_key = 0;

    /* Check appname, keyname != NULL */
    if ((appname == NULL) || (keyname == NULL))
	return (0);

    /* Put the default value in dest */
    if (deflt != NULL) {
	if (strlen (deflt) + 1 > destsize) {
	    strncpy (dest, deflt, destsize - 1);
	    dest[destsize - 1] = '\0';
	} else
	    strcpy (dest, deflt);
    } else
	*dest = '\0';

    /* Open file, return 0 bytes copied if this fails */
    fp = fopen (filename, "rt");
    if (fp == NULL)
	return (0);

    /* Parse the file */
    while (fgets (buf, sizeof (buf), fp) != NULL) {
	if (got_key)
	    break;

	/* Find start of line; skip comment lines */
	line = buf;
	while (isspace((int) *line)) line++;
	if (*line == ';') continue;
	if (*line == '#') continue; /* RD: This won't work with Windows tho! */

	/* Remove trailing spaces, tabs, newlines, returns, etc. */
	for (p = line + strlen (line) - 1; isspace((int) *p); p--) {
	    *p = '\0';
	}

	/* Blank line? */
	if (*line == '\0') continue;

	/* Do we have a "application", i.e. section, name? */
	if ((*line == '[') && (*(line + strlen (line) - 1) == ']')) {
	    line++;
	    *(line + strlen (line) - 1) = '\0';

	    if (strcmp (line, appname) == 0) {
		/* Yes */
		in_section = 1;
	    } else {
		/* No */
		in_section = 0;
	    }

	    continue;
	}

	/* In the right section? */
	if (!in_section) continue;

	/* Is this a key line? */
	if ((p = strchr (line, '=')) != NULL) {
	    /* Split on the equals sign -> line = keyname, p = value */
	    *p = '\0';
	    p++;

	    /* Trim trailing spaces off the key name stored in line. */
	    for (q = line + strlen(line) - 1; isspace((int) *q); q--) {
		*q = '\0';
	    }

	    /* Got the right key? */
	    if (strcmp (keyname, line) != 0) continue;

            /* Skip any blank spaces at the start. */
	    while (isspace((int) *p)) { p++; }

	    /* Check buffer size OK */
	    if ((strlen (p) + 1) > destsize) {
		strncpy (dest, p, destsize - 1);
		dest[destsize - 1] = '\0';
	    } else
		strcpy (dest, p);

	    got_key = 1;
	}
    }

    /* Done */
    fclose (fp);
    return (strlen (dest));
}
