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

	    /*default:                        return(EINVAL); */
	    /* Return the current errno by default */
	default:
	    return (errno);
    }
}
