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
#include <errno.h>

#include <lsck/lsck.h>
#include <lsck/if.h>
#include <lsck/csock.h>
#include "csock.h"

/* TODO: This function in SOCK.VXD seems broken - it succeeds, but no address
 * is returned. Contact SOCK.VXD's author. */

/* -----------------------
 * - __csock_getsockname -
 * ----------------------- */

int __csock_getsockname (LSCK_SOCKET *lsd,
                         struct sockaddr *name, size_t *namelen)
{
	LSCK_SOCKET_CSOCK *csock = (LSCK_SOCKET_CSOCK *) lsd->idata;
	struct sockaddr_in *sock_sa = (struct sockaddr_in *) name;
	struct sockaddr_in *peer_sa = (struct sockaddr_in *) &lsd->peername;
	LSCK_IF_ADDR_INET **inet = NULL;
	ULONG32 sock_addr = 0;
	unsigned short sock_port = 0;
	int ret = 0;
	int i;

	if (*namelen < sizeof (struct sockaddr_in)) {
		errno = ENOBUFS;
		return (-1);
	}

	ret = ___csock_getsockname(csock->fd, &sock_addr, &sock_port);
	if (ret != 0) {
		errno = __csock_errno (ret);
		return (-1);
	}
	*namelen = sizeof (struct sockaddr_in);

	sock_sa->sin_family = AF_INET;
	sock_sa->sin_addr.s_addr = sock_addr;
	sock_sa->sin_port = sock_port;

	/* Just in the case the bug gets fixed. */
	if ((sock_addr != 0) && __lsck_debug_enabled ())
		fputs ("__csock_getsockname() now works!\n", stderr);

	/* TODO: Eliminate this nasty hack. It exists because of the problem
	 * mentioned above. */

	/* Because the getsockname() call above doesn't fill in sock_addr, it
	 * has to be faked using the data we have about the interface. The
	 * following code scans the interface address table for the peer's
	 * network and then returns our interface address on that network. */
	inet = lsd->interface->addr->table.inet;

	for (i = 0, sock_addr = 0; inet[i] != NULL; i++) {
		/* Is this on our network? */
		if ((peer_sa->sin_addr.s_addr & inet[i]->netmask.s_addr)
		    == (inet[i]->addr.s_addr & inet[i]->netmask.s_addr)) {
			sock_addr = inet[i]->addr.s_addr;
			break;
		}

		/* If this network has a gateway, speculatively use this
		 * address. If no other networks match, then this will be
		 used. */
		if (inet[i]->gw_present)
			sock_addr = inet[i]->addr.s_addr;
	}

	sock_sa->sin_addr.s_addr = sock_addr;

	return (0);
}
