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

#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/utsname.h>

int __old_djgpp_libc_uname (struct utsname *u);

/* ---------
   - uname -
   --------- */

/* This uses the old DJGPP uname() routine, but gets the nodename using our
 * gethostname() code. This copies as much of the node's name as possible. */

int uname (struct utsname *u)
{
	int ret, len;
	char hbuf[MAXGETHOSTNAME];
	char *p = NULL;

	/* NOTE: __old_djgpp_libc_uname() should handle u == NULL */
	ret = __old_djgpp_libc_uname(u);
	if (ret != 0) return(ret); /* Pass the error on */

	/* Find the nodename */
	ret = gethostname(hbuf, sizeof(hbuf));
	if (ret != 0) return(ret);

	p = strchr(hbuf, '.');
	if (p != NULL) *p = '\0';

	/* Copy it */
	len = sizeof(u->nodename) - 1; /* nul too */
	if (strlen(hbuf) < len) len = strlen(hbuf);
	strncpy(u->nodename, hbuf, len);
	u->nodename[len] = '\0';

	return(0);
}
