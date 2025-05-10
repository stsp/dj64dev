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
#include <fcntl.h>

#include <lsck/if.h>
#include "csock.h"

/* -----------------
 * - __csock_fcntl -
 * ----------------- */

int __csock_fcntl (LSCK_SOCKET *lsd, int *rv, int command, int request)
{
    int x = 1;
    int fakeflags = 0;
    int handled = 0;	/* Set if we've handled fcntl. */

    switch (command) {
    case F_SETFL:
	    /* Enable/disable non-blocking I/O */
	    x = (request & O_NONBLOCK) == O_NONBLOCK;

	    /* Set/clear non-blocking mode. */
	    handled = __csock_ioctl (lsd, rv, FIONBIO, &x);
	    break;

    case F_GETFL:
	    /* TODO: This should be handled by fcntl emulation. */
	    if (lsd->blocking) fakeflags |= O_NONBLOCK;
	    *rv     = fakeflags;
	    handled = 1;
	    break;

    default:
	    handled = 0;
	    break;
    }

    return(handled);
}
