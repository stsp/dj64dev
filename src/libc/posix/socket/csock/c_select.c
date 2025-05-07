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
 * NOTE: SOCK.VXD uses a socket fd mask here, so one can check for multiple
 * sockets in one call. So, one cannot simply pass a file descriptor here.
 */

#include <string.h>
#include <fcntl.h>
#include <dpmi.h>

#include <sys/socket.h>

#include <lsck/if.h>
#include <lsck/csock.h>
#include "csock.h"

/* TODO: How to specify event? */

/* ------------------
 * - __csock_select -
 * ------------------ */

int __csock_select (LSCK_SOCKET * lsd, int event)
{
	LSCK_SOCKET_CSOCK *csock = (LSCK_SOCKET_CSOCK *) lsd->idata;
	ULONG32 read_fd   = 0;
	ULONG32 write_fd  = 0;
	ULONG32 except_fd = 0;
	ULONG32 bit       = 1UL << csock->fd;
	unsigned to = 0;
	int ret = 0;

	switch (event) {
	case FD_READ:
		read_fd = bit;
		break;

	case FD_WRITE:
		write_fd = bit;
		break;

		/* TODO: Is this correct? Is this how FD_ACCEPT, etc. should be
		 * handled? This solution seems better than previous one of
		 * select'ing on all 3 here. I think only one event is passed
		 * at a time. */
	default:
		except_fd = bit;
		break;
	}

	ret = ___csock_select(&read_fd, &write_fd, &except_fd, &to);
	if (ret != 0) {
		errno = __csock_errno (ret);
		ret = -1;
	} else {
		/* Success */
		if (((read_fd & bit) == bit)
		    || ((write_fd & bit) == bit)
		    || ((except_fd & bit) == bit))
			ret = 1;
		else
			ret = 0;
	}

	return (ret);	/* TODO: Return the elapsed time? */
}

/* -----------------------
 * - __csock_select_wait -
 * ----------------------- */

int __csock_select_wait (LSCK_SOCKET * lsd, int event)
{
	int ret = 0;

	while (ret == 0) {
		ret = __csock_select (lsd, event);
		__dpmi_yield ();
	}

	return (ret);
}
