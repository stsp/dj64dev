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

/* This code was originally rewritten by Indrek Mandre, but was completely
 * rewritten by Richard Dawe to support different network interfaces. */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <io.h>
#include <sys/fsext.h>

#include <sys/socket.h>

#include <lsck/lsck.h>
#include <lsck/if.h>

/* Default protocols for given (domain, type) pairs */
typedef struct _LSCK_SOCKET_DEFAULT {
    int domain;
    int type;
    int protocol;
} LSCK_SOCKET_DEFAULT;

LSCK_SOCKET_DEFAULT __lsck_socket_default[] =
{
    {AF_INET, SOCK_STREAM, IPPROTO_TCP},
    {AF_INET, SOCK_DGRAM, IPPROTO_UDP},
    {0, 0, 0}
};

/* Moved this function to fsext.c -> func. decl. here */
extern int __lsck_socket_fsext (__FSEXT_Fnumber func_number,
				int *rv, va_list args);

/* Socket information */
LSCK_SOCKET *__lsck_socket[__LSCK_NUM_SOCKET];

/* Interface information */
extern LSCK_IF *__lsck_interface[LSCK_MAX_IF + 1];

/* Function declarations */
int __socketx (int domain, int type, int protocol, LSCK_IF *interface);

/* ----------
 * - socket -
 * ---------- */

int socket (int domain, int type, int protocol)
{
	/* Create a socket with no interface preference. */
	return(__socketx(domain, type, protocol, NULL));
}

/* -------------
 * - __socketx -
 * ------------- */

/* This is an internal function, used to support socketpair(). DO NOT USE IT
 * IN PROGRAMS! */

int __socketx (int domain, int type, int protocol, LSCK_IF *interface)
{
	int s = 0;			/* File descriptor for socket   */
	int ret = 0;			/* Return value                 */
	int i = 0;			/* Loop variable                */
	int freelsd = -1;		/* Free socket descriptor       */
	LSCK_SOCKET *lsd = NULL;	/* Pointer to socket descriptor */
	LSCK_IF *my_if = interface;	/* Interface to use to create   */
	int olderrno;

	/* Fail if we can't initialise */
	if (!__lsck_init ()) {
		/* TODO: Not sure which error to return here. */
		errno = ENODEV;
		/*errno = EPROTONOSUPPORT; */
		return (-1);
	}

	/* If protocol is 0, convert it to the default protocol for the given
	 * family & type. If it's a raw socket, just accept the protocol
	 * value. */
	if ((protocol == 0) && (type != SOCK_RAW)) {
		for (i = 0; __lsck_socket_default[i].domain != 0; i++) {
			if (   (__lsck_socket_default[i].domain == domain)
			    && (__lsck_socket_default[i].type == type)) {
				protocol = __lsck_socket_default[i].protocol;
				break;
			}
		}
	}

	/* Allocate a file descriptor */
	s = __FSEXT_alloc_fd (__lsck_socket_fsext);

	if (s < 0) {
		/* No free file descriptors? */
		errno = EMFILE;
		return (-1);
	}
	setmode (s, O_BINARY);

	/* -- Allocate a libsocket socket for the descriptor -- */

	/* Find the first free descriptor */
	for (i = 0; (__lsck_socket[i] != NULL) && (i < __LSCK_NUM_SOCKET); i++)
		{;}

	if ((i == (__LSCK_NUM_SOCKET - 1)) && (__lsck_socket[i] == NULL)) {
		/* The last desciptor can be used, but needs special
		 * checking. */
		freelsd = i;
	} else if (i < __LSCK_NUM_SOCKET) {
		/* Rest are OK */
		freelsd = i;
	}

	/* None free! Free the file descriptor, and return error. */
	if (freelsd < 0) {
		__FSEXT_set_function (s, NULL);
		_close (s);
		errno = EMFILE;
		return (-1);
	}

	lsd = __lsck_socket[freelsd] = malloc(sizeof(LSCK_SOCKET));

	if (lsd == NULL) {
		__FSEXT_set_function(s, NULL);
		_close (s); /* Free the socket descriptor */
		errno = ENOBUFS;
		return (-1);
	}

	memset (lsd, 0, sizeof (LSCK_SOCKET));

	/* Set the file descriptor's data to be the socket descriptor. */
	__FSEXT_set_data(s, (void *) lsd);

/* Define a pseudo-function for clean up. This is complicated by the DJGPP
 * version number :( */
#define __SOCKET_CLEANUP    __FSEXT_set_function(s, NULL);  \
	                    __FSEXT_set_data(s, NULL);      \
                            _close(s);                      \
                            free(lsd);                      \
                            __lsck_socket[freelsd] = NULL

	lsd->family = domain;
	lsd->type = type;
	lsd->protocol = protocol;
	lsd->backlog = 0;		/* Need to listen() to set this.    */
	lsd->blocking = 1;		/* Block by def'lt                  */
	lsd->fd = s;			/* Store file desc.                 */
	lsd->urgent_pid = getpid ();	/* Send SIGURG signals to this pid. */
	lsd->inbound = 1;		/* Allow recv(), recvfrom()         */
	lsd->outbound = 1;		/* Allow send(), sendto()           */

	/* Create the socket on the first matching interface */
	for (i = 0; (my_if == NULL) && (__lsck_interface[i] != NULL); i++) {
		if (__lsck_interface[i]->proto_check == NULL)
			continue;

		if (__lsck_interface[i]->proto_check(domain, type, protocol)) {
			my_if = __lsck_interface[i];
			break;
		}
	}

	/* If the function user spec'd an interface or we found one, use it. */
	lsd->interface = my_if;

	if (lsd->interface != NULL) {
		if (lsd->interface->socket == NULL)
			lsd->interface = NULL;
	}

	if (lsd->interface == NULL) {
		/* No interfaces! */
		__SOCKET_CLEANUP;
		errno = EPROTONOSUPPORT;
		return (-1);
	}

	/* Creation failed, so return error code */
	ret = lsd->interface->socket (lsd);

	if (ret == -1) {
		/* TODO: Fix this hack, which is dodgy. */
		olderrno = errno;
		__SOCKET_CLEANUP;
		errno = olderrno;
	}

	/* Return the descriptor - on error, errno should be set by the
	 * interface's socket() call */
	return ((ret == -1) ? -1 : s);
}

/* ---------------
 * - __fd_to_lsd -
 * --------------- */

LSCK_SOCKET *__fd_to_lsd (int fd)
{
	LSCK_SOCKET *lsd = NULL;

	/* Check the file descriptor refers to a socket. */
	if (!__fd_is_socket(fd)) return(NULL);

	/* Retrieve the fd's associated data. */
	lsd = (LSCK_SOCKET *) __FSEXT_get_data(fd);

	return (lsd);
}
