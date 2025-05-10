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

/*
 * newerror.c - New libsocket errors for libc.
 * Written by Richard Dawe
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>

#include <lsck/lsck.h>

/* ---------------
 * - lsck_perror -
 * --------------- */

/* This is now obsolete. */

/*void lsck_perror (char *s)
{
    fprintf (stderr, "%s: %s\n", s, lsck_strerror (errno));
}*/

/* ------------
 * - strerror -
 * ------------ */

extern char *__old_djgpp_libc_strerror (int errnum);

const char *strerror (int errnum)
{
    /* Return our own error message if there is one, else use the libc
     * routine. */
    switch (errnum) {
        /* This message was taken from DJGPP 2.04. It should be synchronised
	 * with the DJGPP 2.04 (or later) version. */
        case ELOOP:
            return ("Too many levels of symbolic links (ELOOP)");

        case EREMOTE:
            return ("Object is remote (EREMOTE)");

	case EPROTOTYPE:
	    return ("Protocol error (EPROTOTYPE)");

        case EUSERS:
	    return ("Too many users (EUSERS)");

	case ENOTSOCK:
	    return ("Not a socket (ENOTSOCK)");

	case EDESTADDRREQ:
	    return ("Destination address required (EDESTADDRREQ)");

	case EMSGSIZE:
	    return ("Message too long (EMSGSIZE)");

	case ENOPROTOOPT:
	    return ("Protocol not available (ENOPROTOOPT)");

	case EPROTONOSUPPORT:
	    return ("Protocol not supported (EPROTONOSUPPORT)");

	case ESOCKTNOSUPPORT:
	    return ("Socket type not supported (ESOCKTNOSUPPORT)");

	case EOPNOTSUPP:
	    return ("Operation not supported on endpoint (EOPNOTSUPP)");

	case EPFNOSUPPORT:
	    return ("Protocol family not supported (EPFNOSUPPORT)");

	case EAFNOSUPPORT:
	    return ("Address family not supported (EAFNOSUPPORT)");

	case EADDRINUSE:
	    return ("Address already in use (EADDRINUSE)");

	case EADDRNOTAVAIL:
	    return ("Cannot assign requested address (EADDRNOTAVAIL)");

	case ENETDOWN:
	    return ("Network is down (ENETDOWN)");

	case ENETUNREACH:
	    return ("Network is unreachable (ENETUNREACH)");

	case ENETRESET:
	    return ("Network dropped connection because of reset (ENETRESET)");

	case ECONNABORTED:
	    return ("Software caused connection abort (ECONNABORTED)");

	case ECONNRESET:
	    return ("Connection reset by peer (ECONNRESET)");

	case ENOBUFS:
	    return ("No buffer space available (ENOBUFS)");

	case EISCONN:
	    return ("Transport endpoint is already connected (EISCONN)");

	case ENOTCONN:
	    return ("Transport endpoint is not connected (ENOTCONN)");

	case ESHUTDOWN:
	    return ("Cannot send after transport endpoint shutdown "
		    "(ESHUTDOWN)");

        case ETOOMANYREFS:
            return ("Too many references: cannot splice (ETOOMANYREFS)");

	case ETIMEDOUT:
	    return ("Connection timed out (ETIMEOUT)");

	case ECONNREFUSED:
	    return ("Connection refused (ECONNREFUSED)");

	case EHOSTDOWN:
	    return ("Host is down (EHOSTDOWN)");

	case EHOSTUNREACH:
	    return ("No route to host (EHOSTUNREACH)");

        case ESTALE:
	    return ("Stale NFS handle (ESTALE)");

        case EDQUOT:
	    return ("Quota exceeded (EDQUOT)");

	case EALREADY:
	    return("Operation already in progress (EALREADY)");

	case EINPROGRESS:
	    return("Operation now in progress (EINPROGRESS)");

	default:
	    return (__old_djgpp_libc_strerror(errnum));
    }

    /* Safety net */
    return (NULL);
}
