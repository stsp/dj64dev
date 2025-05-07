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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <lsck/lsck.h>
#include <lsck/if.h>

/* -----------
 * - connect -
 * ----------- */

int connect (int s, struct sockaddr *serv_addr, size_t addrlen)
{
	LSCK_SOCKET *lsd;
	int ret;
	size_t sock_addrlen;

	/* Find the socket descriptor */
	lsd = __fd_to_lsd (s);

	if (lsd == NULL) {
		isfdtype(s, S_IFSOCK);
		return (-1);
	}

	/* Check for completion of outstanding non-blocking I/O */
	if (lsd->interface->nonblocking_check != NULL)
		lsd->interface->nonblocking_check (lsd);

	/* Non-blocking socket connecting? */
	if (!lsd->blocking && lsd->connecting) {
		errno = EALREADY;
		return (-1);
	}

	/* Socket already connected? Datagram sockets can call this
	 * multiply. */
	/* TODO: What should the behaviour be for other sockets? */
	if (lsd->connected && (lsd->type == SOCK_STREAM)) {
		errno = EISCONN;
		return (-1);
	}

	/* Is the socket listening? */
	if (lsd->listening) {
		errno = EOPNOTSUPP;
		return(-1);
	}

	/* Check the address length is valid. */
	if ((lsd->type == SOCK_STREAM)
	    || ((lsd->type == SOCK_DGRAM) && (serv_addr != NULL))) {
		if ((lsd->interface->addrlen_check == NULL)
		    || !lsd->interface->addrlen_check(lsd, addrlen)) {
			/* Bad address length! */
			errno = EINVAL;
			return(-1);
		}
	}

	/* For datagrams, just store/clear the address and return. */
	if (lsd->type == SOCK_DGRAM) {
		if (serv_addr != NULL) {
			/* We must be connecting to the same socket address
			 * family! However, since the socket may not have
			 * been bound, we must check against the socket's
			 * family. */
			if (serv_addr->sa_family != lsd->family) {
				errno = EAFNOSUPPORT;
				return (-1);
			}

			memcpy(&lsd->peername, serv_addr, addrlen);
			lsd->peernamelen = addrlen;
		} else {
			bzero(&lsd->peername, __SOCKADDR_MAX_SIZE);
			lsd->peernamelen = 0;
		}
		return(0);
	}

	/* - Now strictly handling stream sockets! - */

	/* Address parameter OK? */
	if (serv_addr == NULL) {
		errno = EFAULT;
		return(-1);
	}

	/* We must be connecting to the same socket address family! However,
	 * since the socket may not have been bound, we must check against the
	 * socket's family. */
	if (serv_addr->sa_family != lsd->family) {
		errno = EAFNOSUPPORT;
		return (-1);
	}

	/* TODO: Need to check the interface's routing table for a route. */

	/* TODO: RD: What was I talking about here? */
	/* NOTE: May have to modify routing code to identify the interface
	 * here. Currently there are only two, so it doesn't matter. */

	/* Use the appropriate interface */
	if (lsd->interface->connect == NULL) {
		errno = EOPNOTSUPP;
		return (-1);
	}
	ret = lsd->interface->connect (lsd, serv_addr, addrlen);

	/* Store the details on success, else clear the socket descriptor. */
	if ((ret == 0)
	    || ((ret == -1) && !lsd->blocking && (errno == EWOULDBLOCK))) {
		/* Store the peer name */
		memcpy (&lsd->peername, serv_addr, addrlen);
		lsd->peernamelen = addrlen;

		/* Now find the socket name. This is performed after the peer
		 * address has been stored, so that the interface code can use
		 * the peer address to obtain the socket name (if
		 * necessary). */
		if ((ret == 0) && (lsd->interface->getsockname != NULL)) {
			sock_addrlen = __SOCKADDR_MAX_SIZE;
			ret = lsd->interface->getsockname
				  (lsd,
				   (struct sockaddr *) &lsd->sockname,
				   &sock_addrlen);
			lsd->socknamelen = sock_addrlen;

			if (ret == -1) {
				bzero (&lsd->sockname,
				       sizeof (struct sockaddr_in));
				/* TODO: Fix this? Let the connect() call
				 * succeed. */
				ret = 0;
			}
		}

		if ((ret == -1) && !lsd->blocking && (errno == EWOULDBLOCK)) {
			/* Non-blocking connects are OK */
			lsd->connecting = 1;
			lsd->connected = 0;
		} else {
			lsd->connecting = 0;
			lsd->connected = 1;
		}
	} else {
		/* Call unsuccessful, so clear details for safety. */
		bzero (&lsd->sockname, sizeof (struct sockaddr));
		bzero (&lsd->peername, sizeof (struct sockaddr));

		lsd->socknamelen = 0;
		lsd->peernamelen = 0;
		lsd->connected = 0;
	}

	/* Return the correct error if non-blocking connect & still waiting */
	if ((ret == -1) && !lsd->blocking && (errno == EWOULDBLOCK)) {
		errno = EINPROGRESS;
		return (-1);
	}
	return (ret);
}
