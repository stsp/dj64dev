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

#include <stdlib.h>
#include <string.h>

#include <dpmi.h>
#include <sys/movedata.h>
#include <sys/segments.h>

#include <lsck/if.h>
#include <lsck/csock.h>
#include "csock.h"

/* ----------------
 * - __csock_recv -
 * ---------------- */

ssize_t __csock_recv (LSCK_SOCKET *lsd,
                      void *buf, size_t len, unsigned int flags)
{
	LSCK_SOCKET_CSOCK *csock = (LSCK_SOCKET_CSOCK *) lsd->idata;
	ssize_t       ret;
	ULONG32       recvlen;

	/* Check flags */
	if (flags != 0) {
		/* Unsupported flag used */
		errno = EINVAL;
		return(-1);
	}

	ret = ___csock_recv(csock->fd, buf, len, &recvlen);
	if (ret < 0) {
		errno = __csock_errno (ret);
		return -1;
	}
	return recvlen;
}

/* --------------------
 * - __csock_recvfrom -
 * -------------------- */

ssize_t __csock_recvfrom (LSCK_SOCKET *lsd,
                          void *buf, size_t len, unsigned int flags,
		          struct sockaddr *from, socklen_t *fromlen)
{
	LSCK_SOCKET_CSOCK *csock = (LSCK_SOCKET_CSOCK *) lsd->idata;
	ssize_t ret;
	ULONG32 recvlen;
	struct sockaddr_in from_sin;
	int flag;
	int rv;

	/* Check the size fo the socket address passed. */
	if ((from != NULL) && (*fromlen < sizeof (from_sin))) {
		errno = EINVAL;
		return (-1);
	}

	/* Check flags */
	if ( (flags != 0) && ((flags & ~MSG_PEEK) != 0) ) {
		/* Unsupported flag used */
		errno = EINVAL;
		return(-1);
	}

	/* NOTE: SOCK.VXD has a bug in its recvfrom() code. If a socket is set
	 * to non-blocking mode, then recvfrom() will block, and vice-versa.
	 * So, before performing the call, invert the mode. Reset the mode
	 * after the call. */

	/* RD: Later: Version 1 of SOCK.VXD has this bug fixed. */
	if (__csock_version < 1) {
		flag = lsd->blocking;
		__csock_ioctl (lsd, &rv, FIONBIO, &flag);
	}

	ret = ___csock_recvfrom(csock->fd, buf, len,
			&recvlen, (ULONG32 *)&from_sin.sin_addr.s_addr,
			&from_sin.sin_port);

	from_sin.sin_family = AF_INET;

	if (__csock_version < 1) {
		flag = !flag;
		__csock_ioctl (lsd, &rv, FIONBIO, &flag);
	}

	/* Copy the peer's name, if there is one. */
	if (from != NULL) {
		/* Unix98 spec says: truncate if addrlen not large enough */
		if (*fromlen > sizeof(from_sin)) *fromlen = sizeof(from_sin);
		memcpy (from, &from_sin, *fromlen);
	}

	if (ret < 0) {
		errno = __csock_errno (ret);
		return -1;
	}
	return recvlen;
}
