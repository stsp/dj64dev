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

/* hostname.c - Network-aware host, node and domain name functions. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>

#include <lsck/lsck.h>
#include <lsck/if.h>
#include <lsck/domname.h>
#include <lsck/hostname.h>
#include <lsck/ini.h>		       /* .ini and .cfg functions       */

static char hostname[MAXGETHOSTNAME];
static char nodename[MAXGETHOSTNAME];
static char domainname[MAXGETHOSTNAME];
static int gothostname   = 0;
static int gotnodename   = 0;
static int gotdomainname = 0;

extern LSCK_IF *__lsck_interface[LSCK_MAX_IF + 1];

int __old_djgpp_libc_gethostname (char *buf, int size);

/* ---------------
 * - gethostname -
 * --------------- */
#if 0
int gethostname (char *buf, int size)
{
	LSCK_IF *interface = NULL;
	LSCK_IF_ADDR_INET *inet = NULL;
	char *p = NULL, *q = NULL;
	int i, j;
	int ret;

	/* Check the size is valid */
	if (size <= 0) {
		errno = EINVAL;
		return (-1);
	}

	/* Check that buf is valid */
	if (buf == NULL) {
		errno = EINVAL;
		return (-1);
	}

	/* Try to get the host name. Try to obtain it from the following
	 * sources:
	 *
	 * 1. from the HOSTNAME environment variable, to allow the user to
	 *    override settings;
	 *
	 * 2. from the main section of the configuration file;
	 *
	 * 3. from the interface data;
	 *
	 * 4. from the old DJGPP gethostname() function, which gets 1 or the
	 *    NetBIOS name.
	 */

	/* Check if we've already got it */
	if (gothostname) p = strdup(hostname);
	if (p == NULL) p = strdup(getenv ("HOSTNAME"));

	if (p == NULL) {
		/* Allocate memory */
		p = (char *) malloc (MAXGETHOSTNAME);
		if (p == NULL) {
			errno = ENOMEM;
	    		return(-1);	       /* malloc failed */
		}

		/* Get configuration details */
		q = __lsck_config_getfile();

		if (q != NULL) {
	    		ret = GetPrivateProfileString("main", "hostname", NULL,
			                              p, MAXGETHOSTNAME, q);

	    		/* Host name not present in configuration file */
			if (ret == 0) {
				free (p);
				p = NULL;
	    		}
		} else {
			free(p);
			p = NULL;
		}
	}

	/* Scan interfaces */
	if (p == NULL) {
		for (i = 0; (interface = __lsck_interface[i]) != NULL; i++) {
			if (p != NULL) break;
			if (interface->addr == NULL) continue;
			if (interface->addr->family != AF_INET) continue;

			for (j = 0;
			     (inet = interface->addr->table.inet[j]) != NULL;
			     j++) {
				if (strlen (inet->hostname) == 0) continue;
				p = strdup (inet->hostname);
				break;
	    		}
		}
	}

	/* Failed - try DJGPP gethostname */
	if (p == NULL) {
		p = (char *) malloc (MAXGETHOSTNAME);
		if (p == NULL) {
	    		errno = ENOMEM;
			return(-1); /* malloc failed */
		}
		__old_djgpp_libc_gethostname (p, MAXGETHOSTNAME);
	}

	/* Don't allow spaces in the host name! Terminate the string at the
	 * first one found. Also, lower case the whole string. */
	q = strchr (p, ' ');
	if (q != NULL) *q = '\0';
	strlwr (p);

	/* Copy the host name */
	if (((int)strlen (p) + 1) > size) {
		/* Not enough space! */
		errno = EINVAL;
		return (-1);
	}
	strcpy (buf, p);

	/* Save a local copy of the host, node and domain names. */
	strcpy(hostname, p);
	gothostname = 1;

	strcpy(nodename, p);
	gotnodename = 1;

	q = strchr(nodename, '.');
	if (q != NULL) {
		*q = '\0';
		q++;
		strcpy(domainname, q);
		gotdomainname = 1;
	}

	/* Done */
	free (p);
	return (0);
}
#endif

/* ---------------
 * - sethostname -
 * --------------- */

int sethostname (char *buf, int size)
{
	char *q = NULL;

	/* Check the size is valid */
	if ((size <= 0) || (size > MAXGETHOSTNAME)) {
		errno = EINVAL;
		return (-1);
	}

	/* Set the host name */
	strcpy (hostname, buf);
	gothostname = 1;

	strcpy(nodename, buf);
	gotnodename = 1;

	q = strchr(nodename, '.');
	if (q != NULL) {
		*q = '\0';
		q++;
		strcpy(domainname, q);
		gotdomainname = 1;
	}

	/* OK */
	return (0);
}

/* -----------------
 * - getdomainname -
 * ----------------- */

int getdomainname (char *name, size_t len)
{
	/* TODO: I don't think the upper bound is right? */
	/* Fail automatically if len <= 0 or len >= MAXGETDOMAINNAME */
	if ((len <= 0) || (len > MAXGETDOMAINNAME)) {
		errno = EINVAL;
		return (-1);
	}

	/* Check the buffer is valid */
	if (name == NULL) {
		errno = EFAULT;
		return (-1);
	}

	/* OK */
	if (gotdomainname) strcpy(name, domainname); else name[0] = '\0';
	return(0);
}

/* -----------------
 * - setdomainname -
 * ----------------- */

int setdomainname (const char *name, size_t len)
{
	/* Fail automatically if len <= 0 or len >= MAXGETDOMAINNAME */
	if ((len <= 0) || (len > MAXGETDOMAINNAME)) {
		errno = EINVAL;
		return (-1);
	}

	/* Check the buffer is valid */
	if (name == NULL) {
		errno = EFAULT;
		return (-1);
	}

	/* Store the domain name & rebuild the host name. */
	strcpy(domainname, name);
	gotdomainname = 1;

	strcpy(hostname, nodename);
	strcat(hostname, ".");
	strcat(hostname, domainname);
	gothostname = 1;

	/* OK */
	return(0);
}
