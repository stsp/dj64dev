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
 * i_ntop.c
 *
 * This module implements the inet_ntop() function, which converts addresses
 * from binary (numeric) to text (presentation) format.
 *
 */

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

/* -------------
 * - inet_ntop -
 * ------------- */

const char *inet_ntop (int af, const void *src, char *dst, size_t size)
{
	char *res = NULL; /* Fail by default */
	const char *p   = (const char *) src;

	/* Check we've been passed a valid buffer. */
	if (dst == NULL) {
		errno = EFAULT;
		return(NULL);
	}

	/* Process the address family appropriately. */
	switch(af) {
	/* IPv4 */
	case AF_INET:
		/* dst must point to a string buffer large enough to
		 * accommodate the converted address -
		 *   INET_ADDRSTRLEN
		 * = 16 bytes
		 * = (3 * 3 digits) + 3 dots + nul. */
		if (size < INET_ADDRSTRLEN) {
			errno = ENOSPC;
			break;
		}

		sprintf(dst, "%d.%d.%d.%d",
				((int) p[0] & 0xff), ((int) p[1] & 0xff),
				((int) p[2] & 0xff), ((int) p[3] & 0xff));
		res = dst;
		break;

	/* Unsupported address family */
	default:
		errno = EAFNOSUPPORT;
		break;
	}

	return((const char *) res);
}
