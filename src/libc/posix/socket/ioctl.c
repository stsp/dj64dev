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
#include <string.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <ioctls.h>		       /* For Linux ioctl support */
#include <net/if.h>		       /* For Linux ioctl support */

#include <lsck/lsck.h>
#include <lsck/if.h>

int ioctlsocket_emulated (LSCK_SOCKET * lsd, int request, int *param);

/* ---------------
 * - ioctlsocket -
 * --------------- */

int ioctlsocket (int s, int request, int *param)
{
	LSCK_SOCKET *lsd;
	int handled = 0;       	/* If the interface handled it, set to 1. */
	int rv;
	int ret;

	/* Find the socket descriptor */
	lsd = __fd_to_lsd (s);

	if (lsd == NULL) {
		isfdtype(s, S_IFSOCK);
		return (-1);
	}

	/* Check for completion of outstanding non-blocking I/O */
	if (lsd->interface->nonblocking_check != NULL)
		lsd->interface->nonblocking_check (lsd);

	/* Check param */
	if (param == NULL) {
		errno = EFAULT;
		return(-1);
	}

	/* Use appropriate interface */
	if (lsd->interface->ioctl != NULL) {
		/* If the ioctl is not supported by the interface, this should
		 * return 0, so that it can be emulated. */
		handled = lsd->interface->ioctl(lsd, &rv, request, param);
	}

	/* Try to emulate it if the call failed. */
	if (!handled)
		ret = ioctlsocket_emulated (lsd, request, param);
	else
		ret = rv;

	return (ret);
}

/* ------------------------
 * - ioctlsocket_emulated -
 * ------------------------ */

int ioctlsocket_emulated (LSCK_SOCKET * lsd, int request, int *param)
{
	LSCK_IF_ADDR_INET *inet = NULL;
	struct ifreq *ifr = NULL;
	struct sockaddr_in *sin = NULL;
	struct sockaddr_in *sin2 = NULL;
	char *str = NULL;
	int ret = -1;		/* Fail by default */
	int i;

	switch (request) {
	/* Interface name ioctl */
	case SIOCGIFNAME:
		str = (char *) param;
		if (lsd->interface->name != NULL) {
			strcpy (str, lsd->interface->name);
			ret = 0;
		} else {
			errno = EINVAL;
		}
		break;

	/* - ioctls returning "struct ifreq *" - */

	/* Local socket address */
	case SIOCGIFADDR:
		/* Internet domain? */
		if (lsd->sockname.sa_family != AF_INET) {
			errno = EINVAL;
			break;
		}

		/* Stream or datagram? */
		if ((lsd->type != SOCK_STREAM) && (lsd->type != SOCK_DGRAM)) {
			errno = EINVAL;
			break;
		}

		/* Stream socket connected? */
		if (    (lsd->type == SOCK_STREAM)
		    && !lsd->accepted
		    && !lsd->connected
		    && !lsd->connecting) {
			errno = ENOTCONN;
			break;
		}

		/* Datagram socket bound? */
		if (   (lsd->type == SOCK_DGRAM)
		    && !lsd->bound) {
			/* TODO: Right error? */
			errno = EAGAIN;
			break;
		}

		ifr = (struct ifreq *) param;
		memcpy (&ifr->ifr_ifru.ifru_addr,
			&lsd->sockname, sizeof (struct sockaddr));

		ret = 0;
		break;

	/* Peer socket address */
	case SIOCGIFDSTADDR:
		/* Internet domain? */
		if (lsd->sockname.sa_family != AF_INET) {
			errno = EINVAL;
			break;
		}

		/* Stream or datagram? NB: "connected" dgram has a desination
		 * address associated with it. */
		if ((lsd->type != SOCK_STREAM) && (lsd->type != SOCK_DGRAM)) {
			errno = EINVAL;
			break;
		}

		/* Stream socket connected? */
		if (   (lsd->type == SOCK_STREAM)
		    && !lsd->accepted
		    && !lsd->connected) {
			errno = ENOTCONN;
			break;
		}

		/* Datagram socket connected? */
		if (   (lsd->type == SOCK_DGRAM)
		    && lsd->peernamelen) {
		       errno = EDESTADDRREQ;
		       break;
		}

		ifr = (struct ifreq *) param;
		memcpy (&ifr->ifr_ifru.ifru_dstaddr,
			&lsd->peername, sizeof (struct sockaddr));

		ret = 0;
		break;

	/* Subnet mask */
	case SIOCGIFNETMASK:
	        /* Internet domain? */
		if (lsd->sockname.sa_family != AF_INET) {
			errno = EINVAL;
			break;
		}

		/* Stream or datagram? NB: "connected" dgram has a desination
		 * address associated with it. */
		if ((lsd->type != SOCK_STREAM) && (lsd->type != SOCK_DGRAM)) {
			errno = EINVAL;
			break;
		}

		/* Stream socket connected? */
		if (   (lsd->type == SOCK_STREAM)
		    && !lsd->accepted
		    && !lsd->connected) {
			errno = ENOTCONN;
			break;
		}

		/* Datagram socket connected? */
		if (   (lsd->type == SOCK_DGRAM)
		    && lsd->peernamelen) {
		       /* TODO: This is probably the wrong error to use here,
			* but what error should be used? This makes a strong
			* distinction between streams and datagrams. */
		       errno = EDESTADDRREQ;
		       break;
		}

		ifr = (struct ifreq *) param;
		bzero (ifr, sizeof (*ifr));
		sin = (struct sockaddr_in *) &lsd->sockname;

		/* Scan the interface to find the netmask for the socket
		 * address. */
		if ((lsd->interface->addr != NULL)
		    && (lsd->interface->addr->family == AF_INET)) {
			for (i = 0;
			     (inet = lsd->interface->addr->table.inet[i])
				     != NULL;
			     i++) {
				if (sin->sin_addr.s_addr
				    == inet->addr.s_addr) {
					sin2 = (struct sockaddr_in *)
						&ifr->ifr_ifru.ifru_netmask;
					sin2->sin_family = AF_INET;
					sin2->sin_addr.s_addr
						= inet->netmask.s_addr;
					break;
				}
			}
		} else {
			errno = EINVAL;
			break;
		}

		ret = 0;
		break;

	case SIOCGIFFLAGS:
	case SIOCGIFBRDADDR:	       /* TODO: Broadcast address? */
	case SIOCGIFMETRIC:
	case SIOCGIFMEM:
	case SIOCGIFMTU:
	default:
		errno = EINVAL;
		break;
	}

	/* Done */
	return (ret);
}
