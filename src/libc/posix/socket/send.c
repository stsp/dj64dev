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

#include <signal.h>
#include <string.h>

#include <sys/socket.h>

#include <lsck/lsck.h>
#include <lsck/if.h>

/* --------
 * - send -
 * -------- */

ssize_t send (int s, const void *msg, size_t len, unsigned int flags)
{
    return (sendto(s, msg, len, flags, NULL, 0));
}

/* ----------
 * - sendto -
 * ---------- */

ssize_t sendto (int s, const void *msg, size_t len, unsigned int flags,
                struct sockaddr *to, socklen_t tolen)
{
    LSCK_SOCKET *lsd;
    ssize_t ret;
    struct sockaddr *my_to    = to;
    size_t           my_tolen = tolen;

    /* Find the socket descriptor */
    lsd = __fd_to_lsd (s);

    if (lsd == NULL) {
        isfdtype(s, S_IFSOCK);
	return (-1);
    }

    /* Check for completion of outstanding non-blocking I/O */
    if (lsd->interface->nonblocking_check != NULL)
	lsd->interface->nonblocking_check (lsd);

    /* NOTE: sendto() doesn't need the socket to be in a connect'd state. */

    /* No buffer */
    if (msg == NULL) {
	errno = EFAULT;
	return (-1);
    }

    /* Check destination params */
    if ( (to != NULL) && (tolen == 0) ) {
	errno = EINVAL;
	return (-1);
    }

    /* sendto() allowed? */
    if (!lsd->outbound) {
	raise(SIGPIPE);
	errno = EPIPE;
	return(-1);
    }

    /* Stream sockets */
    if (lsd->type == SOCK_STREAM) {
	/* Not connected */
	if (!lsd->connected) {
	    errno = ENOTCONN;
	    return(-1);
	}

	/* Destination address spec'd? */
	if (my_to == NULL) {
	    my_to    = &lsd->peername;
	    my_tolen = lsd->peernamelen;
	}

	/* Barf if the destination address is not the peer's address. */
	/* TODO: Should there be an error here even if it's the same?
	 * AlainM wasn't sure. */
	if (   (my_tolen != lsd->peernamelen)
	    || (memcmp(&lsd->peername, my_to, my_tolen) != 0) ) {
	    errno = EISCONN;
	    return(-1);
        }

	/* Are we allowed to send? */
	if (!lsd->outbound) {
		raise(SIGPIPE);
		errno = EPIPE;
		return(-1);
	}

        /* Use the appropriate transport. Because we don't know how the
	 * interface implements a sendto, use a send function for the stream-
	 * type socket. */
        if (lsd->interface->send == NULL) {
	    errno = EOPNOTSUPP;
	    return (-1);
        }

        ret = lsd->interface->send (lsd, msg, len, flags);
        return (ret);
    }

    /* Datagram sockets */
    if ( (lsd->type == SOCK_DGRAM) && (my_to == NULL) ) {
	if (lsd->peername.sa_family == 0) {
	    /* No address data! */
	    errno = EDESTADDRREQ;
	    return(-1);
	}

	/* Use address assigned by earlier connect() call */
	my_to    = &lsd->peername;
	my_tolen = lsd->peernamelen;
    }

    /* Are we allowed to send? */
    if (!lsd->outbound) {
	errno = EPIPE;
	return(-1);
    }

    /* Use the appropriate transport */
    if (lsd->interface->sendto == NULL) {
	errno = EOPNOTSUPP;
	return (-1);
    }

    ret = lsd->interface->sendto (lsd, msg, len, flags, my_to, my_tolen);
    return (ret);
}

/* -----------
 * - sendmsg -
 * ----------- */

ssize_t sendmsg (int s, const struct msghdr *msg, int flags)
{
	errno = EOPNOTSUPP;
	return(-1);
}
