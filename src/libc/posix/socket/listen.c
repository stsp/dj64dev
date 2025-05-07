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

#include <sys/socket.h>

#include <lsck/lsck.h>
#include <lsck/if.h>

/* ----------
 * - listen -
 * ---------- */

int listen (int s, int backlog)
{
	LSCK_SOCKET *lsd;
	int ret = -1;		/* Fail by default */

	/* Find the socket descriptor */
	lsd = __fd_to_lsd (s);

	if (lsd == NULL) {
		isfdtype(s, S_IFSOCK);
		return (-1);
	}

	/* Check for completion of outstanding non-blocking I/O */
	if (lsd->interface->nonblocking_check != NULL)
		lsd->interface->nonblocking_check (lsd);

	/* If it's not a stream socket, we can't listen. */
	if (lsd->type != SOCK_STREAM) {
		errno = EOPNOTSUPP;
		return (-1);
	}

	/* If it's not bound, then this is invalid. */
	if (!lsd->bound) {
		errno = EINVAL;
		return (-1);
	}

	/* Listen */
	if (lsd->interface->listen == NULL) {
		errno = EOPNOTSUPP;
		return (-1);
	}
	ret = lsd->interface->listen (lsd, backlog);

	/* Store the new backlog */
	if (ret != -1) {
		lsd->listening = 1;
		lsd->backlog = backlog;
	}

	/* Done */
	return (ret);
}
