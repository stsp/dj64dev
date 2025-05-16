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

/* --------
 * - recv -
 * -------- */

ssize_t recv (int s, void *buf, size_t len, unsigned int flags)
{
    LSCK_SOCKET *lsd;
    ssize_t ret;

    /* Find the socket descriptor */
    lsd = __fd_to_lsd (s);

    if (lsd == NULL) {
	isfdtype(s, S_IFSOCK);
	return (-1);
    }
    /* Check for completion of outstanding non-blocking I/O */
    if (lsd->interface->nonblocking_check != NULL)
	lsd->interface->nonblocking_check (lsd);

    /* Connect'd? */
    if ((lsd->type == SOCK_STREAM) && !lsd->connected) {
	errno = ENOTCONN;
	return (-1);
    }

    /* No buffer */
    if (buf == NULL) {
	errno = EFAULT;
	return (-1);
    }

    /* recv() allowed? */
    if (!lsd->inbound)
	return (0);

    /* Use the appropriate interface */
    if (lsd->interface->recv == NULL) {
	errno = EOPNOTSUPP;
	return (-1);
    }
    ret = lsd->interface->recv (lsd, buf, len, flags);
    return (ret);
}

/* ------------
 * - recvfrom -
 * ------------ */

ssize_t recvfrom (int s, void *buf, size_t len, unsigned int flags,
                  struct sockaddr *from, socklen_t *fromlen)
{
    LSCK_SOCKET *lsd;
    ssize_t ret;

    /* Find the socket descriptor */
    lsd = __fd_to_lsd (s);

    if (lsd == NULL) {
        isfdtype(s, S_IFSOCK);
	return (-1);
    }
    /* Check for completion of outstanding non-blocking I/O */
    if (lsd->interface->nonblocking_check != NULL)
	lsd->interface->nonblocking_check (lsd);

    /* No buffer */
    if (buf == NULL) {
	errno = EFAULT;
	return (-1);
    }

    /* NB: from can be NULL */
    if ( (from != NULL) && (fromlen == NULL) ) {
	errno = EFAULT;
	return (-1);
    }

    /* recvfrom() allowed? */
    /* TODO: This should return an error, but what error? */
    if (!lsd->inbound) return (0);

    /* Use the appropriate interface */
    if (lsd->interface->recvfrom == NULL) {
	errno = EOPNOTSUPP;
	return (-1);
    }

    ret = lsd->interface->recvfrom (lsd, buf, len, flags, from, fromlen);
    return (ret);
}

/* -----------
 * - recvmsg -
 * ----------- */

ssize_t recvmsg (int s, struct msghdr *msg, int flags)
{
	errno = EOPNOTSUPP;
	return(-1);
}
