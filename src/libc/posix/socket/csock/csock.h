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

#ifndef __libsocket_csock_h__
#define __libsocket_csock_h__

#ifdef __cplusplus
extern "C"
{
#endif

#include <dpmi.h>
#include <sys/socket.h>

#include <lsck/if.h>

/* --- Defines --- */

/* Error codes, taken from sock.h from Coda's sock.vxd source */
#define CSOCK_ERR_INVALID_PARAM       0x00000100
#define CSOCK_ERR_NO_MEMORY           0x00000200
#define CSOCK_ERR_INVALID_SOCKET      0x00000300
#define CSOCK_ERR_ALREADY_BOUND       0x00000400
#define CSOCK_ERR_NOT_BOUND           0x00000500
#define CSOCK_ERR_ACCESS              0x00000600
#define CSOCK_ERR_INTERNAL            0x00000700
#define CSOCK_ERR_FD_INUSE            0x00000800
#define CSOCK_ERR_INFINITE_WAIT       0x00000900
#define CSOCK_ERR_NOT_CONNECTED       0x00000a00
#define CSOCK_ERR_WOULD_BLOCK         0x00000b00
#define CSOCK_ERR_NOT_LISTENING       0x00000c00

/* Minimum error code - used to distinguish sock.vxd errors from TDI ones. */
#define CSOCK_ERR_MIN CSOCK_ERR_INVALID_PARAM

/* From the SOCK.VXD sources, the following sizes are defined: */
#define CSOCK_RECV_BUFSIZE	32768		/* Receive buffer size */
#define CSOCK_SEND_LOWWATER	1024
#define CSOCK_SEND_MAXSIZE	5000

/* --- Structures --- */

/* SOCK.VXD socket information */
typedef struct _LSCK_SOCKET_CSOCK {
	int fd;				/* SOCK.VXD's unique file desc. */
} LSCK_SOCKET_CSOCK;

/* --- Functions --- */
extern LSCK_IF *__csock_init (void);

extern int __csock_uninit (void);

extern int __csock_errno (int /* i_errno */);

extern int __csock_proto_check (int /* domain */,
				int /* type */,
				int /* protocol */);

extern int __csock_addrlen_check (LSCK_SOCKET * /* lsd */,
				  size_t /* addrlen */);

extern int __csock_socket (LSCK_SOCKET * /* lsd */);

extern int __csock_socketpair (LSCK_SOCKET * /* lsd */ [2]);

extern int __csock_close (LSCK_SOCKET * /* lsd */);

extern int __csock_bind (LSCK_SOCKET *     /* lsd */,
                         struct sockaddr * /* my_addr */,
			 size_t            /* addrlen */);

extern int __csock_accept (LSCK_SOCKET *     /* lsd */,
			   LSCK_SOCKET *     /* nsd */,
			   struct sockaddr * /* sockaddr */,
			   size_t *          /* addrlen */);

extern int __csock_listen (LSCK_SOCKET * /* lsd */, int /* backlog */);

extern int __csock_connect (LSCK_SOCKET *     /* lsd */,
			    struct sockaddr * /* serv_addr */,
			    size_t            /* addrlen */);

extern ssize_t __csock_recv (LSCK_SOCKET * /* lsd */,
			     void *        /* buf */,
			     size_t        /* len */,
                             unsigned int  /* flags */);

extern ssize_t __csock_recvfrom (LSCK_SOCKET *     /* lsd */,
				 void *            /* buf */,
				 size_t            /* len */,
                                 unsigned int      /* flags */,
                                 struct sockaddr * /* from */,
				 size_t *          /* fromlen */);

extern ssize_t __csock_send (LSCK_SOCKET * /* lsd */,
			     const void *  /* msg */,
			     size_t        /* len */,
                             unsigned int  /* flags */);

extern ssize_t __csock_sendto (LSCK_SOCKET *     /* lsd */,
			       const void *      /* msg */,
			       size_t            /* len */,
                               unsigned int      /* flags */,
                               struct sockaddr * /* to */,
			       size_t            /* tolen */);

extern int __csock_getpeername (LSCK_SOCKET *     /* lsd */,
                                struct sockaddr * /* name */,
				size_t *          /* namelen */);

extern int __csock_getsockname (LSCK_SOCKET *     /* lsd */,
				struct sockaddr * /* name */,
				size_t *          /* namelen */);

extern int __csock_shutdown (LSCK_SOCKET * /* lsd */, int /* how */);

extern int __csock_select (LSCK_SOCKET * /* lsd */, int /* event */);

extern int __csock_select_wait (LSCK_SOCKET * /* lsd */, int /* event */);

extern int __csock_ioctl (LSCK_SOCKET * /* lsd */,
			  int *         /* rv */,
			  int           /* request */,
			  int *         /* param */);

extern int __csock_fcntl (LSCK_SOCKET * /* lsd */,
			  int *         /* rv */,
			  int           /* command */,
			  int           /* request */);

extern int __csock_nonblocking_check (LSCK_SOCKET * /* lsd */);

extern int __csock_getsockopt (LSCK_SOCKET * /* lsd */,
			       int *         /* rv */,
			       int           /* level */,
			       int           /* optname */,
                               void *        /* optval */,
			       size_t *      /* optlen */);

/* File descriptor tracking */
extern void __csock_fd_set_usage (int /* fd */, int /* flag */);
extern void __csock_fd_set_used  (int /* fd */);
extern void __csock_fd_set_clear (int /* fd */);
extern int  __csock_fd_get_usage (int /* fd */);

/* --- Globals --- */
//extern int csock_entry[2];
extern char CSOCK_IF_NAME[];
extern unsigned int __csock_version;

#ifdef __cplusplus
};
#endif

#endif /* __libsocket_csock_h__ */

