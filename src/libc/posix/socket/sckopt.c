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
 *  This library is free software; you 2can redistribute it and/or modify it
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

/* INFO: Comments starting with 'IM' indicate that Indrek Mandre made changes.
 * Comments starting wiht 'RD' indicate that Richard Dawe made changes. If no
 * comment is given, then the code was written by Indrek Mandre. */

#include <string.h>

#include <sys/socket.h>

#include <lsck/lsck.h>
#include <lsck/if.h>

/* Emulation functions */
int getsockopt_emulated (LSCK_SOCKET *lsd, int level, int optname,
			 void *optval, socklen_t *optlen);

int setsockopt_emulated (LSCK_SOCKET *lsd, int level, int optname,
			 const void *optval, socklen_t optlen);

/* --------------
 * - getsockopt -
 * -------------- */

int getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen)
{
	LSCK_SOCKET *lsd;
	void   *my_optval = optval;
	socklen_t *my_optlen = optlen;
	char    buf[16];		/* Temp. buffer if optval == NULL  */
	socklen_t  buflen = sizeof(buf);	/* Temp. buffer size               */
	int handled = 0;		/* If interface handled, set to 1. */
	int rv;
	int ret = -1;			/* Fail by default                 */

	/* Find the socket descriptor */
	lsd = __fd_to_lsd (s);

	if (lsd == NULL) {
		isfdtype(s, S_IFSOCK);
		return (-1);
	}

	/* Check for completion of outstanding non-blocking I/O */
	if (lsd->interface->nonblocking_check != NULL)
		lsd->interface->nonblocking_check (lsd);

	/* Check parameters */
	if ( (optval != NULL) && (optlen == NULL) ) {
		errno = EFAULT;
		return(-1);
	}

	if (optval == NULL) {
		/* Pass the interface our temporary buffer, so it doesn't
		 * have to worry about valid addresses. */
		my_optval = (void *) buf;
		my_optlen = &buflen;
	}

	/* Use appropriate interface */
	if (lsd->interface->getsockopt != NULL) {
		handled = lsd->interface->getsockopt(lsd, &rv,
						     level, optname,
						     my_optval, my_optlen);
	}

	/* If the interface hasn't handled the option, emulate it. */
	if (!handled)
		ret = getsockopt_emulated(lsd,
					  level, optname,
					  my_optval, my_optlen);
	else
		ret = rv;

	return (ret);
}

/* -----------------------
 * - getsockopt_emulated -
 * ----------------------- */

int getsockopt_emulated (LSCK_SOCKET *lsd, int level, int optname,
			 void *optval, socklen_t *optlen)
{
	int *i_optval = (int *) optval;
	int ret = 0;	/* Succeed by default. */

	/* Only emulate socket-level options for now. */
	if (level != SOL_SOCKET) {
		errno = ENOPROTOOPT;
		return(-1);
	}

	/* Check buffer space */
	switch(optname) {
	/* Integer values */
	case SO_ACCEPTCONN:
	case SO_TYPE:
		if (*optlen < sizeof(int)) {
			errno = ENOBUFS;
			return(-1);
		}
		break;
	}

	switch(optname) {
	/* Socket is listening */
	case SO_ACCEPTCONN:
		*i_optval = lsd->listening;
		*optlen   = sizeof(int);
		break;

	/* Socket type */
	case SO_TYPE:
		*i_optval = lsd->type;
		*optlen   = sizeof(int);
		break;

	default:
		errno = ENOPROTOOPT;
		ret   = -1;
		break;
	}

	return(ret);
}

/* --------------
 * - setsockopt -
 * -------------- */

int setsockopt (int s, int level, int optname, const void *optval,
                socklen_t optlen)
{
	LSCK_SOCKET *lsd;
	int handled = 0;	/* If the interface handled it, set to 1. */
	int rv;
	int ret = -1;

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
	if (optval == NULL) {
		errno = EFAULT;
		return(-1);
	}

	/* Use appropriate interface */
	if (lsd->interface->setsockopt != NULL) {
		handled =  lsd->interface->setsockopt(lsd, &rv,
						      level, optname,
						      optval, optlen);
	}

	if (!handled)
		ret = setsockopt_emulated(lsd, level, optname, optval, optlen);
	else
		ret = rv;

	return (ret);
}

/* -----------------------
 * - setsockopt_emulated -
 * ----------------------- */

int setsockopt_emulated (LSCK_SOCKET *lsd, int level, int optname,
			 const void *optval, socklen_t optlen)
{
	errno = ENOPROTOOPT;
	return(-1);
}
