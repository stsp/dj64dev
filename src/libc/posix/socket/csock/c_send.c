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
#include <stdlib.h>

#include <dpmi.h>
#include <sys/movedata.h>
#include <sys/segments.h>

#include <netinet/in.h>

#include <lsck/if.h>
#include <lsck/csock.h>
#include "csock.h"

/* ----------------
 * - __csock_send -
 * ---------------- */

ssize_t __csock_send (LSCK_SOCKET *lsd,
                      const void *msg, size_t len, unsigned int flags)
{
	LSCK_SOCKET_CSOCK *csock = (LSCK_SOCKET_CSOCK *) lsd->idata;
	ssize_t ret;
	ULONG32 sentlen;

	/* No flags are supported */
	if (flags != 0) {
		errno = EINVAL;
		return(-1);
	}

	ret = ___csock_send(csock->fd, msg, len, &sentlen);
	/* NOTE: Carry is always set, because SOCK.VXD's sys_send() function
	 * always returns a value - length, error, whatever - so SOCK.VXD
	 * errors must be distinguished from TDI ones! */
	if (ret < 0) {
		if (-ret < CSOCK_ERR_MIN) ret = -ret; /* TDI error code */
		errno = __csock_errno (ret);
		ret = -1;
	}

	/* NOTE: SOCK.VXD returns the sent length in EAX & ECX. ECX, however,
	 * is bitwise OR'd with some kind of status byte, so it's value may
	 * differ from the actual sent length. So, use EAX here. */

	/*else {
	  ret = sentlen;
	  }*/

	return (ret);
}

/* ------------------
 * - __csock_sendto -
 * ------------------ */

ssize_t __csock_sendto (LSCK_SOCKET *lsd,
                        const void *msg, size_t len, unsigned int flags,
		        struct sockaddr *to, socklen_t tolen)
{
	LSCK_SOCKET_CSOCK *csock = (LSCK_SOCKET_CSOCK *) lsd->idata;
	ssize_t ret;
	ULONG32 sentlen;
	struct sockaddr_in bind_sin;
	socklen_t bind_sin_len;
	struct sockaddr_in *to_sa = (struct sockaddr_in *) to;

	/* If this is a datagram socket, bind to a local address, if
	 * necessary. */
	if ( (lsd->type == SOCK_DGRAM) && !lsd->bound) {
		/* TODO: Is this correct? Should it not bind only to the IP
		 * address that will be used to send the data? If so, how do
		 * you unbind it? Do you just bind to another address? Would a
		 * sendto() with an address followed by another sendto()
		 * without an address lead to this situation: bind() with
		 * address, bind() with wildcard address? */

		/* Bind it to _any_ local address. */
		bzero(&bind_sin, sizeof(bind_sin));
		bind_sin.sin_family      = AF_INET;
		bind_sin.sin_port        = 0;
		bind_sin.sin_addr.s_addr = INADDR_ANY;

		ret = __csock_bind(lsd,
				   (struct sockaddr *) &bind_sin,
				   sizeof(bind_sin));

		/* Copy the details into socket descriptor, if successful,
		 * else nuke. Since we've called __csock_bind() here, we have
		 * to handle what bind() would normally do for us. */
		if (ret == 0) {
			/* Get the local address. */

			/* TODO: This probably shouldn't ignore the return
			 * address. However, how can we unbind the socket on
			 * error? So for the moment, possibly return a bogus
			 * address. */
			bind_sin_len = sizeof(bind_sin);
			__csock_getsockname(lsd,
					    (struct sockaddr *) &bind_sin,
					    &bind_sin_len);

			memcpy(&lsd->sockname,
			       (void *) &bind_sin, bind_sin_len);
			lsd->socknamelen = bind_sin_len;
			lsd->bound = 1;
		} else {
			bzero (&lsd->sockname, sizeof (struct sockaddr));

			lsd->socknamelen = 0;
			lsd->bound = 0;
		}

		/* TODO: This should probably translate some of the errors. */
		if (ret == -1) return(-1);
	}

	/* No flags are supported */
	if (flags != 0) {
		errno = EINVAL;
		return(-1);
	}

	ret = ___csock_sendto(csock->fd, to_sa->sin_addr.s_addr,
			to_sa->sin_port, msg, len, &sentlen);
	/* NOTE: Carry is always set, because SOCK.VXD's sys_sendto() function
	 * always returns a value - length, error, whatever - so SOCK.VXD
	 * errors must be distinguished from TDI ones! */
	if (ret < 0) {
		if (-ret < CSOCK_ERR_MIN) ret = -ret; /* TDI error code */
		errno = __csock_errno (ret);
		return -1;
	}
	return sentlen;
}
