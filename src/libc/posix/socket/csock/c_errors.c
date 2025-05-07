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
#include <sys/socket.h>

#include "csock.h"

/* Cheers Microsoft */
#include "tdistat.h"

/* -----------------
 * - __csock_errno -
 * ----------------- */

int __csock_errno (int i_errno)
{
    switch (i_errno) {
	    /* SOCK.VXD errors */
	case -CSOCK_ERR_INVALID_PARAM:
	    return (EINVAL);
	case -CSOCK_ERR_NO_MEMORY:
	    return (ENOBUFS);
	case -CSOCK_ERR_INVALID_SOCKET:
	    return (ENOTSOCK);
	case -CSOCK_ERR_ALREADY_BOUND:
	    return (EINVAL);
	case -CSOCK_ERR_NOT_BOUND:
	    /* RD: What was I thinking here? */
	    /*return (EDESTADDRREQ);*/
	    /* This a bit generic, but it's probably the most sensible error
	     * return here. */
	    return(EINVAL);
	case -CSOCK_ERR_ACCESS:
	    return (EACCES);
	case -CSOCK_ERR_INTERNAL:
	    return (EAGAIN);	       /* TODO: Hmmm */
	case -CSOCK_ERR_FD_INUSE:
	    return (EBUSY);	       /* TODO: Hmmm */
	case -CSOCK_ERR_INFINITE_WAIT:
	    return (EBUSY);	       /* TODO: Hmmm */
	case -CSOCK_ERR_NOT_CONNECTED:
	    return (ENOTCONN);
	case -CSOCK_ERR_WOULD_BLOCK:
	    return (EWOULDBLOCK);

	    /* Errors SOCK.VXD has passed through from TDI */
	case TDI_SUCCESS:
	    return (0);		       /* No error! */
	case TDI_NO_RESOURCES:
	    return (ENOBUFS);
	case TDI_ADDR_IN_USE:
	    return (EADDRINUSE);
	    /* TDI_NO_FREE_ADDR */
	    /* TDI_ADDR_INVALID */
	    /* TDI_ADDR_DELETED */
	    /* TDI_BUFFER_OVERFLOW */
	    /* TDI_BAD_EVENT_TYPE */
	    /* TDI_BAD_OPTION */
	case TDI_CONN_REFUSED:
	    return (ECONNREFUSED);
	case TDI_INVALID_CONNECTION:
	    return (ENOTCONN);
	    /* TDI_ALREADY_ASSOCIATED */
	    /* TDI_NOT_ASSOCIATED */
	case TDI_CONNECTION_ACTIVE:
	    return (EISCONN);
	case TDI_CONNECTION_ABORTED:
	    return (ECONNREFUSED);
	case TDI_CONNECTION_RESET:
	    return (ECONNRESET);
	case TDI_TIMED_OUT:
	    return (ETIMEDOUT);
	case TDI_GRACEFUL_DISC:
	    return (ECONNRESET);
	    /* TDI_NOT_ACCEPTED */
	    /* TDI_MORE_PROCESSING */
	    /* TDI_INVALID_STATE */
	    /* TDI_INVALID_PARAMETER */
	case TDI_DEST_NET_UNREACH:
	    return (ENETUNREACH);
	case TDI_DEST_HOST_UNREACH:
	case TDI_DEST_PORT_UNREACH: /* TODO: Hmm */
	    return (EHOSTUNREACH);
	    /* TDI_DEST_PROT_UNREACH */
	    /* TDI_INVALID_QUERY */
	    /* TDI_BUFFER_TOO_SMALL */
	    /* TDI_CANCELLED */
	    /* TDI_BUFFER_TOO_BIG */
	    /* TDI_INVALID_REQUEST */
	    /* TDI_PENDING */

	    /*default:                        return(EINVAL); */
	    /* Return the current errno by default */
	default:
	    return (errno);
    }
}
