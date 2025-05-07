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
 * config.c - libsocket configuration functions
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lsck/lsck.h>

static char *configdir = NULL;	/* Where the configuration files are     */
static int configdirset = 0;	/* 1 => configdir has been set, 0 => not */

static char *configfile = NULL;
static int configfileset = 0;

#define LSCK_CONFIG_FILE "lsck.cfg"

/* ------------------------
 * - __lsck_config_getdir -
 * ------------------------ */

/* This returns the directory name that libsocket will look in for its config.
 * files. */

char *__lsck_config_getdir (void)
{
    char *p = NULL;

    /* Have we set configdir already? */
    if (!configdirset) {
	/* Try the libsocket environment variable, then the Windows dir, else
	 * leave NULL */
	configdir = getenv ("LSCK");
	if (configdir == NULL) configdir = getenv ("LIBSOCKET");
	if (configdir == NULL) return(NULL);

	/* Maybe this isn't such a good idea. */
	/*if (configdir == NULL) configdir = getenv("windir"); */

	/* Done this now */
	configdirset = 1;
    }
    /* Forward-slashify the directory name */
    for (p = configdir; *p != '\0'; p++) {
	if (*p == '\\')
	    *p = '/';
    }

    return (configdir);
}

/* ------------------------
 * - __lsck_config_setdir -
 * ------------------------ */

/* This sets the directory that libsocket looks for configuration files. I
 * guess this isn't particularly useful, but it's here for symmetry (spot the
 * physicist :) ). The old directory is returned. */

char *__lsck_config_setdir (char *newdir)
{
    char *p = NULL, *q = NULL;

    p = configdir;
    configdir = strdup (newdir);
    for (q = configdir; *q != '\0'; q++) {
	if (*q == '\\')
	    *q = '/';
    }
    configdirset = 1;
    return (p);			       /* Return the saved old directory */
}

/* -------------------------
 * - __lsck_config_getfile -
 * ------------------------- */

/* This returns the filename that libsocket should use. If none has been set,
 * then the default is used. */

char *__lsck_config_getfile (void)
{
    if (!configfileset) {
	configfile = malloc (PATH_MAX);
	if (configfile == NULL) return (NULL);

	/* Default configuration file */
	if (__lsck_config_getdir() == NULL) return(NULL);
	sprintf(configfile, "%s/%s", __lsck_config_getdir(), LSCK_CONFIG_FILE);
	configfileset = 1;
    }
    return (configfile);
}

/* -------------------------
 * - __lsck_config_setfile -
 * ------------------------- */

/* This sets the filename that libsocket should use for its configuration
 * file. */

char *__lsck_config_setfile (char *newfile)
{
    char *p = NULL;

    /* Copy the configuration filename */
    p = configfile;
    configfile = newfile;
    configfileset = 1;

    /* Return the old pointer */
    return (p);
}
