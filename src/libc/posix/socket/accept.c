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
 *  under the terms of the GNU Library General Public Lice2nse as published
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

/* This was written by Indrek Mandre, extensively rewritten by Richard Dawe
 * (RD). There are suggestions by Alain Magloire (AM), implemented by RD. */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <io.h>
#include <sys/fsext.h>

#include <sys/socket.h>

#include <lsck/lsck.h>
#include <lsck/if.h>

extern int __lsck_socket_fsext (__FSEXT_Fnumber func_number,
				int *rv, va_list args);

extern LSCK_SOCKET *__lsck_socket[__LSCK_NUM_SOCKET];

/* ----------
 * - accept -
 * ---------- */

int accept (int s, struct sockaddr *addr, size_t *addrlen)
{
	LSCK_SOCKET *lsd;		/* Current socket descriptor */
	LSCK_SOCKET *nsd;		/* New socket descriptor     */
	int newfd;			/* New file descriptor       */
	int ret;			/* Return value              */
	int i;			/* Loop variable             */
	int freelsd;		/* Free socket descriptor    */

	/* Find the socket descriptor */
	lsd = __fd_to_lsd (s);

	if (lsd == NULL) {
		isfdtype(s, S_IFSOCK);
		return (-1);
	}

	/* Check for completion of outstanding non-blocking I/O */
	if (lsd->interface->nonblocking_check != NULL)
		lsd->interface->nonblocking_check (lsd);

	/* Unsupported socket type */
	if (   (lsd->type != SOCK_STREAM)
	    || (lsd->interface->accept == NULL)) {
		errno = EOPNOTSUPP;
		return (-1);
	}

	/* Can't accept on sockets that were created via accept(), non-bound
	 * or non-listening sockets! */
	if (lsd->accepted || !lsd->bound || !lsd->listening) {
		errno = EINVAL;
		return (-1);
	}

	/* Check address parameters */
	if (   (addr == NULL)
	    || ( (addr != NULL) && (addrlen == NULL) ) ) {
		errno = EFAULT;
		return(-1);
	}

	/* Create a descriptor */
	newfd = __FSEXT_alloc_fd (__lsck_socket_fsext);

	if (newfd < 0) {
		errno = ENFILE;
		return (-1);
	}
	setmode (newfd, O_BINARY);

	/* -- Allocate a libsocket socket for the accept'd descriptor -- */

	/* Find the first free descriptor */
	for (i = 0;
	     (__lsck_socket[i] != NULL) && (i < __LSCK_NUM_SOCKET);
	     i++) {
		;
	}

	freelsd = -1;

	if ((i == (__LSCK_NUM_SOCKET - 1)) && (__lsck_socket[i] == NULL)) {
		/* The last desciptor can be used, but needs special
		 * checking. */
		freelsd = i;
	} else if (i < __LSCK_NUM_SOCKET) {
		/* Rest are OK */
		freelsd = i;
	}

	/* None free! Free the file descriptor, and return error. */
	if (freelsd < 0) {
		__FSEXT_set_function (newfd, NULL);
		_close (newfd);
		/* Alain Magloire: POSIX says ENFILE instead of EMFILE */
		errno = ENFILE;
		return (-1);
	}

	nsd = __lsck_socket[freelsd] = malloc (sizeof (LSCK_SOCKET));

	if (nsd == NULL) {
		__FSEXT_set_function (newfd, NULL);
		_close (newfd);
		errno = ENOBUFS;
		return (-1);
	}

	/* Set the file descriptor's data to be the socket descriptor. */
	__FSEXT_set_data(newfd, (void *) nsd);

	/* Create a socket with *almost* the same properties */
	memcpy (nsd, lsd, sizeof (LSCK_SOCKET));
	nsd->fd        = newfd;
	nsd->listening = 0;

	/* Accept on the appropriate interface. */
	nsd->peernamelen = __SOCKADDR_MAX_SIZE;
	ret = lsd->interface->accept (lsd, nsd,
				      &nsd->peername, &nsd->peernamelen);

	if (ret == 0) {
		/* Call successful */

		/* Store peer details, if desired */
		if (addr != NULL) {
			/* Unix98 spec says: truncate if addrlen not large
			 * enough */
			if (*addrlen > nsd->peernamelen)
				*addrlen = nsd->peernamelen;
			memcpy(addr, &nsd->peername, *addrlen);
		}

		/* Now we've accepted, stop any accept calls on the new
		 * descriptor. */
		/* We've accepted, so we're also connected. */
		nsd->accepted = 1;
		nsd->connected = 1;
	} else {
		/* Call unsuccessful - tidy up */
		__FSEXT_set_data(newfd, NULL);
		__FSEXT_set_function (newfd, NULL);
		_close (newfd);
		free(nsd->idata);
		nsd->idata = NULL;
		free (nsd);
		__lsck_socket[freelsd] = NULL;
	}

	return ((ret == -1) ? -1 : nsd->fd);
}
