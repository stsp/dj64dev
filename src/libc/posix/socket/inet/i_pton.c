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
 * i_pton.c
 *
 * This module implements the inet_ntop() function, which converts addresses
 * from text (presentation) to binary (numeric) format.
 *
 */

#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

/* -------------
 * - inet_pton -
 * ------------- */

int inet_pton (int af, const char *src, void *dst)
{
	int ret = 0; /* Fail by default */

	switch(af) {
	/* IPv4 address */
	case AF_INET:
		/* Use the normal IPv4 conversion routine. */
		if (inet_aton(src, (struct in_addr *) dst) != 0) {
			/* Success */
			ret = 1;
		}
		break;

	/* Unsupported address format */
	default:
		errno = EAFNOSUPPORT;
		ret   = -1;
		break;
	}

	return(ret);
}
