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

#include <time.h>
#include <string.h>
#include <assert.h>
#include <lsck/if.h>
#include <lsck/csock.h>
#include "csock.h"

/* ----------------------
 * - __csock_getsockopt -
 * ---------------------- */

int __csock_getsockopt (LSCK_SOCKET *lsd, int *rv,
			int level, int optname,
                        void *optval, socklen_t *optlen)
{
	LSCK_SOCKET_CSOCK *csock = (LSCK_SOCKET_CSOCK *) lsd->idata;
	int handled = 0;	/* Set if we've handled the option here. */
	int *i_optval = (int *) optval;
	struct timeval *tv = (struct timeval *) optval;

	/* TODO: Should this let the emulation do its biz? */
	/* Only socket level supported */
	if (level != SOL_SOCKET) {
		errno = EINVAL;
		*rv   = -1;
		return(1);
	}

	/* Buffer size checking */
	switch(optname) {
	/* Integer/boolean options */
	case SO_REUSEADDR:
	case SO_RCVBUF:
	case SO_RCVLOWAT:
	case SO_SNDBUF:
	case SO_SNDLOWAT:
		if (*optlen < sizeof(int)) {
			errno = ENOBUFS;
			*rv   = -1;
			return(1);
		}
		break;

	/* struct timeval options */
	case SO_RCVTIMEO:
	case SO_SNDTIMEO:
		if (*optlen < sizeof(struct timeval)) {
			errno = ENOBUFS;
			*rv   = -1;
			return(1);
		}
		break;
	}

	switch(optname) {
	/* SOCK.VXD always allows address use for bind() calls - see the call
	 * to TdiOpenAddressEntry() in do_bind() in the SOCK.VXD sources. */
	case SO_REUSEADDR:
		*i_optval = 1;
		*optlen   = sizeof(int);
		*rv       = 0;
		handled   = 1;
		break;

	/* Receive buffer size - see sys_socket() in SOCK.VXD sources. */
	case SO_RCVBUF:
		*i_optval = CSOCK_RECV_BUFSIZE;
		*optlen   = sizeof(int);
		*rv       = 0;
		handled   = 1;
		break;

	/* Receive low water mark - see sys_recv() and sys_recvfrom() in
	 * SOCK.VXD sources. Any data received is always returned, so the
	 * low water mark is 1 byte. */
	case SO_RCVLOWAT:
		*i_optval = 1;
		*optlen   = sizeof(int);
		*rv       = 0;
		handled   = 1;
		break;

	/* SOCK.VXD always completes non-blocking receives. */
	case SO_RCVTIMEO:
		tv->tv_sec = tv->tv_usec = 0;
		*rv        = 0;
		handled    = 1;
		break;

	/* Lie about the send buffer size. Since SOCK.VXD uses a linked-list
	 * of data to send, this doesn't really make any difference. It seems
	 * reasonable to pretend it's the same size as the receive buffer. */
	case SO_SNDBUF:
		*i_optval = CSOCK_RECV_BUFSIZE;
		*optlen   = sizeof(int);
		*rv       = 0;
		handled   = 1;
		break;

	/* Send low water mark - see sys_socket() in SOCK.VXD sources. */
	case SO_SNDLOWAT:
		*i_optval = CSOCK_SEND_LOWWATER;
		*optlen   = sizeof(int);
		*rv       = 0;
		handled   = 1;
		break;

	/* SOCK.VXD always completes non-blocking sends. */
	case SO_SNDTIMEO:
		tv->tv_sec = tv->tv_usec = 0;
		*rv        = 0;
		handled    = 1;
		break;

	case SO_ERROR: {
		ULONG32 val;
		int rc;
		assert(*optlen == sizeof(val));
		rc = ___csock_getsoerr(csock->fd, &val);
		if (!rc) {
			memcpy(optval, &val, *optlen);
			*rv = 0;
		} else {
			errno = __csock_errno (rc);
			*rv = -1;
		}
		handled = 1;
		break;
	}

	/* Unknown option */
	default:
		handled = 0;
	}

	return(handled);
}

int __csock_setsockopt (LSCK_SOCKET *lsd, int *rv,
			int level, int optname,
                        const void *optval, socklen_t optlen)
{
	LSCK_SOCKET_CSOCK *csock = (LSCK_SOCKET_CSOCK *) lsd->idata;
	int handled = 0;	/* Set if we've handled the option here. */

	if (level != SOL_SOCKET) {
		errno = EINVAL;
		*rv   = -1;
		return(1);
	}

	/* Buffer size checking */
	switch(optname) {
		case SO_LINGER: {
			int rc;
			const struct linger *lin = (const struct linger *)optval;
			assert(optlen == sizeof(*lin));
			rc = ___csock_setsolinger(csock->fd, lin->l_onoff, lin->l_linger);
			if (rc) {
				errno = __csock_errno(rc);
				*rv = -1;
			} else {
				*rv = 0;
			}
			handled = 1;
			break;
		}
	}
	return handled;
}
