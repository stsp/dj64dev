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

/* Originally by Indrek Mandre (IM), modifications by Richard Dawe (RD) */

#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>

#include <lsck/lsck.h>
#include <lsck/if.h>

extern LSCK_SOCKET *__lsck_socket[__LSCK_NUM_SOCKET];

/* ---------------
 * - closesocket -
 * --------------- */

int closesocket (int s)
{
	LSCK_SOCKET *lsd;
	int ret;
	int i;

	/* Find the socket descriptor */
	lsd = __fd_to_lsd (s);

	if (lsd == NULL) {
		isfdtype(s, S_IFSOCK);
		return (-1);
	}

	/* Check for completion of outstanding non-blocking I/O */
	if (lsd->interface->nonblocking_check != NULL)
		lsd->interface->nonblocking_check (lsd);

	/* Close  */
	if (lsd->interface->close == NULL) {
		errno = EOPNOTSUPP;
		return (-1);
	}
	ret = lsd->interface->close (lsd);

	/* Free memory if successful */
	if (ret == 0) {
		/* Free the interface data. */
		free(lsd->idata);
		lsd->idata = NULL;

		for (i = 0; i < __LSCK_NUM_SOCKET; i++) {
			if (lsd == __lsck_socket[i]) {
				__lsck_socket[i] = NULL;
				free (lsd);
				break;
			}
		}
	}

	return (ret);
}
