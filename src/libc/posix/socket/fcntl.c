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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <lsck/lsck.h>
#include <lsck/if.h>

int fcntlsocket_emulated (LSCK_SOCKET * lsd, int command, int request);

/* ---------------
 * - fcntlsocket -
 * --------------- */

int fcntlsocket (int s, int command, int request)
{
	LSCK_SOCKET *lsd;
	int handled = 0; /* Set if the interface handled the request. */
	int ret, rv;

	/* Find the socket descriptor */
	lsd = __fd_to_lsd (s);

	if (lsd == NULL) {
		isfdtype(s, S_IFSOCK);
		return (-1);
	}

	/* Check for completion of outstanding non-blocking I/O */
	if (lsd->interface->nonblocking_check != NULL)
		lsd->interface->nonblocking_check (lsd);

	/* Process the command appropriately */
	if (lsd->interface->fcntl != NULL) {
		/* If the fcntl is not supported by the interface, this should
		 * return 0, so it can be emulated. */
		handled = lsd->interface->fcntl(lsd, &rv, command, request);
	}

	/* Try to emulate it if the call failed */
	if (!handled)
		ret = fcntlsocket_emulated (lsd, command, request);
	else
		ret = rv;

	/* TODO: The flags from F_GETFL should be as used by open(). However,
	 * the socket is created using socket(), so some "reasonable" flags
	 * should probably be OR'd into ret here. */

	return (ret);
}

/* ------------------------
 * - fcntlsocket_emulated -
 * ------------------------ */

int fcntlsocket_emulated (LSCK_SOCKET *lsd, int command, int request)
{
	int ret = -1;

	switch (request) {
	/* F_GETOWN, F_SETOWN are needed to implement SIGURG support.
	 * See fcntl(2) for details. */

	case F_GETOWN:
		ret = lsd->urgent_pid;
		break;

	case F_SETOWN:
		/* DJGPP's libc can only send signals to current process at the
		 * moment, so F_SETOWN is meaningless unless the pid refers to
		 * the current process / process group (of 1 process). */
		if ((request != getpid ())
		    && (request != -getpid ())) {
			/* TODO: Is this the correct error? */
			errno = EPERM;
		} else {
			lsd->urgent_pid = request;
			ret = 0;
		}
		break;

	default:
		errno = EINVAL;
		break;
	}

	return (ret);
}
