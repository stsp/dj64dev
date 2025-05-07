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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>

#include <lsck/lsck.h>
#include <lsck/if.h>

/* --------
 * - bind -
 * -------- */

int bind (int s, struct sockaddr *my_addr, size_t addrlen)
{
	/* Cope with long addresses. */
	char             buf[__SOCKADDR_MAX_SIZE];
	struct sockaddr *bound_addr    = (struct sockaddr *) buf;
	size_t           bound_addrlen = sizeof(buf);

	LSCK_SOCKET *lsd;
	int ret;

	/* Find the socket descriptor */
	lsd = __fd_to_lsd (s);

	if (lsd == NULL) {
		isfdtype(s, S_IFSOCK);
		return (-1);
	}

	/* Check for completion of outstanding non-blocking I/O */
	if (lsd->interface->nonblocking_check != NULL)
		lsd->interface->nonblocking_check (lsd);

	/* Already bound? */
	if (lsd->bound) {
		errno = EINVAL;
		return (-1);
	}

	/* No address given? */
	if (my_addr == NULL) {
		errno = EFAULT;
		return(-1);
	}

	/* Trying to bind to a different address family? */
	if (lsd->family != my_addr->sa_family) {
		errno = EAFNOSUPPORT;
		return(-1);
	}

	/* TODO: According to Unix98, this error indicates that the socket is
	 * listening and cannot be connected. Do something about this! Some
	 * other error will have to be found, because all socket functions
	 * return this error if the interface doesn't provide a function. */

	/* NOTE: We may have to add some routing here if more than two
	 * interfaces are present. */
	if (lsd->interface->bind == NULL) {
		errno = EOPNOTSUPP;
		return (-1);
	}
	/* Bind */
	ret = lsd->interface->bind (lsd, my_addr, addrlen);

	/* Copy the details into socket descriptor, if successful, else
	 * nuke. */
	if (ret == 0) {
		/* Get the local address. */

		/* TODO: This probably shouldn't ignore the return code.
		 * However, how can we unbind the socket on error? So for the
		 * moment, possibly return a bogus address. */
		getsockname(lsd->fd, bound_addr, &bound_addrlen);

		memcpy (&lsd->sockname, (void *) bound_addr, bound_addrlen);
		lsd->socknamelen = bound_addrlen;
		lsd->bound = 1;
	} else {
		bzero (&lsd->sockname, sizeof (struct sockaddr));

		lsd->socknamelen = 0;
		lsd->bound = 0;
	}

	return (ret);
}
