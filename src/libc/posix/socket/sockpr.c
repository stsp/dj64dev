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
#include <unistd.h>

#include <sys/socket.h>
#include <lsck/errno.h>
#include <lsck/lsck.h>

#include <lsck/if.h>

/* Interface information */
extern LSCK_IF *__lsck_interface[LSCK_MAX_IF + 1];

/* Internal create function */
extern int __socketx (int domain, int type, int protocol, LSCK_IF *interface);

/* --------------
 * - socketpair -
 * -------------- */

int socketpair (int domain, int type, int protocol, int sv[2])
{
	int          my_fd[2]  = { -1, -1 };
	LSCK_SOCKET *my_lsd[2] = { NULL, NULL };
	LSCK_IF     *my_if     = NULL;
	int          olderrno, ret, i;

	if (!__lsck_init ()) {
		/* TODO: Not sure which error to return here. */
		errno = ENODEV;
		/*errno = EPROTONOSUPPORT; */
		return (-1);
	}

	/* Check to see if we can create socket pairs first. Find the
	 * interface that supports the protocol combo and __x_socketpair(). */
	for (i = 0; (my_if == NULL) && (__lsck_interface[i] != NULL); i++) {
		if (__lsck_interface[i]->proto_check == NULL)
			continue;

		if (!__lsck_interface[i]->proto_check(domain, type, protocol))
			continue;

		if (__lsck_interface[i]->socketpair != NULL) {
			my_if = __lsck_interface[i];
			break;
		}
	}

	if (my_if == NULL) {
		errno = EOPNOTSUPP;
		return(-1);
	}

	/* Now create two sockets on the found interface. */
	my_fd[0] = __socketx(domain, type, protocol, my_if);
	my_fd[1] = __socketx(domain, type, protocol, my_if);

	if ((my_fd[0] == -1) || (my_fd[1] == -1)) {
		if (my_fd[0] >= 0) close(my_fd[0]);
		if (my_fd[1] >= 0) close(my_fd[1]);
		return(-1);	/* Just pass errno on */
	}

	/* Find the socket descriptors. */
	my_lsd[0] = __fd_to_lsd(my_fd[0]);
	my_lsd[1] = __fd_to_lsd(my_fd[1]);

	if ((my_lsd[0] == NULL) || (my_lsd[1] == NULL)) {
		/* What's going on here? */
		close(my_fd[0]);
		close(my_fd[1]);
		errno = ENOMEM;
		return(-1);
	}

	/* TODO: Is this split of work good? Or should there be some emulation
	 * code too? I can see that TCP/IP connections could randomly pick
	 * local ports on the loopback address. Hmmm. */

	/* Now let the interface connect the two sockets together. */
	ret = my_if->socketpair(my_lsd);

	/* If the call fails, kill the sockets. Else, set up the socket vector
	 * array and get the socket name and peer name. */
	if (ret == -1) {
		olderrno = errno;
		close(my_fd[0]);
		close(my_fd[1]);
		errno = olderrno;
	} else {
		sv[0] = my_fd[0];
		sv[1] = my_fd[1];

		/* Socket & peer names */
		/* TODO: This should probably check return values. */
		my_lsd[0]->socknamelen
			= my_lsd[0]->peernamelen
			= __SOCKADDR_MAX_SIZE;

		getsockname(my_lsd[0]->fd,
			    &my_lsd[0]->sockname,
			    &my_lsd[0]->socknamelen);

		getpeername(my_lsd[0]->fd,
			    &my_lsd[0]->peername,
			    &my_lsd[0]->peernamelen);

		my_lsd[1]->socknamelen
			= my_lsd[1]->peernamelen
			= __SOCKADDR_MAX_SIZE;

		getsockname(my_lsd[1]->fd,
			    &my_lsd[1]->sockname,
			    &my_lsd[1]->socknamelen);

		getpeername(my_lsd[1]->fd,
			    &my_lsd[1]->peername,
			    &my_lsd[1]->peernamelen);
	}

	return(ret);
}
