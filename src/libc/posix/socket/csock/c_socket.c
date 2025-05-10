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
#include <errno.h>

#include <lsck/if.h>
#include <lsck/csock.h>
#include "csock.h"

/* ------------------
 * - __csock_socket -
 * ------------------ */

int __csock_socket (LSCK_SOCKET *lsd)
{
	LSCK_SOCKET_CSOCK *csock = NULL;
	int flag = 0;
	int ret = -1;
	int rv;
	unsigned int i;

	/* Allocate a buffer for the interface data. */
	csock = (LSCK_SOCKET_CSOCK *) malloc(sizeof(*csock));
	lsd->idata = (void *) csock;

	if (csock == NULL) {
		errno = ENOMEM;
		return(-1);
	}

	ret = ___csock_open(lsd->protocol, &i);
	/* Got one! */
	if (ret == 0) {
		csock->fd = i;
	}

	if (csock->fd == -1) {
		/* No free descriptors! */
		/* TODO: Does SOCK.VXD allocate per process or per system? If
		 * it's per system, then this needs to be changed to ENFILE. */
		errno = EMFILE;
		return (-1);
	}

	if (ret != 0) {
		errno = __csock_errno (ret);
		return (-1);
	}

	__csock_ioctl (lsd, &rv, FIONBIO, &flag);	/* Blocking I/O */

	return (0);
}
