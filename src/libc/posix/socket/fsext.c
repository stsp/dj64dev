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
 * fsext.c
 *
 * File system extensions for treating sockets like ordinary file descriptors.
 * Moved here by Rich Dawe from Indrek Mandre's socket.c.
 */

#include <sys/fsext.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <sys/socket.h>

#include <lsck/lsck.h>

/* RD: If this returns 1, it informs DJGPP that it emulated the required
 * file system function. The actual return value is stored in rv. */

/* -----------------------
 * - __lsck_socket_fsext -
 * ----------------------- */

int __lsck_socket_fsext (__FSEXT_Fnumber func_number, int *rv, va_list args)
{
	int s;
	void *data;
	int count;
	int t = 0, r = 0, selectsocket_err = 0;
	int req;
	int *ioctl_comm;
	int fcntl_comm;
	struct stat *st;

	s = va_arg (args, int);

	switch (func_number) {
	case __FSEXT_read:
		data = va_arg (args, void *);
		count = va_arg (args, int);

		/* RD: Non-blocking I/O should return 0 if no bytes were read,
		 * and set errno to EAGAIN! */
		*rv = recv (s, data, count, 0);

		if ((*rv == -1) && (errno == EWOULDBLOCK)) {
			*rv = 0;
			errno = EAGAIN;
		}
		return(1);

	case __FSEXT_write:
		data = va_arg (args, void *);
		count = va_arg (args, int);

		/* RD: Try and fix write'ing in the same way as read'ing - I
		 * don't know whether send() via WSOCK.VXD will ever return
		 * the blocking I/O error, but this should cope with it. */
		*rv = send (s, data, count, 0);

		if ((*rv == -1) && (errno = EWOULDBLOCK)) {
		    *rv = 0;
		    errno = EAGAIN;
		}
		return(1);

	case __FSEXT_ready:
		/*
		 * RD: Removed |FD_ACCEPT from first selectsocket() call.
		 *
		 * Also removed 'else if ( t <= 0 ) r |= __FSEXT_ready_error;'
		 * from the 'if (t > 0)'. The code now detects a failure in
		 * selectsocket() & returns an error. Previously it would just
		 * signal that the socket had an error, which is misleading and
		 * also not often checked by programs using select().
		 */

		/* TODO: Fix this code properly. The problem is that FD_ACCEPT,
		 * etc. are not well handled by each interface function. This
		 * could probably be fixed. */

		t = selectsocket (s, FD_READ);	/* Ready for reading? */
		if (t > 0)
			r |= __FSEXT_ready_read;
		else if (t == -1)
			selectsocket_err = 1;

		t = selectsocket (s, FD_WRITE);	/* Ready for writing? */
		if (t > 0)
			r |= __FSEXT_ready_write;
		else if (t == -1)
			selectsocket_err = 1;

		/* RD: Error condition on socket? */
		t = selectsocket (s, ~(FD_READ | FD_WRITE));	/* Dodgy? */
		if (t > 0)
			r |= __FSEXT_ready_error;
		else if (t == -1)
			selectsocket_err = 1;

		*rv = selectsocket_err ? -1 : r;
		/*return r?r:8; *//* IM: Must I here return the value? */
		/* RD: No, just non-zero, so the FS extensions know we handled
		 * it. */
		return(1);

	case __FSEXT_close:
	    *rv = closesocket(s);
	    if (*rv == 0) {
		__FSEXT_set_function(s, NULL);
		__FSEXT_set_data(s, NULL);
		_close(s);
	    }
	    return(1);

	case __FSEXT_ioctl:
		req = va_arg (args, int);
		ioctl_comm = va_arg(args, int *);

		*rv = ioctlsocket (s, req, ioctl_comm);
		return(1);

	case __FSEXT_fcntl:
		fcntl_comm = va_arg (args, int);

		switch(fcntl_comm) {
		/* Set flags */
		case F_SETFL:
			req = va_arg(args, int);
			*rv = fcntlsocket(s, fcntl_comm, req);
			break;

		/* Get flags */
		case F_GETFL:
			*rv = fcntlsocket(s, fcntl_comm, 0);
			break;

		/* Set owner - for SIGURG support */
		case F_SETOWN:
			req = va_arg(args, int);
			*rv = fcntlsocket(s, fcntl_comm, req);
			break;

		/* Get owner - for SIGURG support */
		case F_GETOWN:
			*rv = fcntlsocket(s, fcntl_comm, 0);
			break;

		/* Unsupported command */
		default:
			*rv = -1;
			break;
		}

		return(1);

	/* lseek() is not supported on sockets - where would you move the
	 * file pointer to? */
	case __FSEXT_lseek:
		errno = ESPIPE;
		*rv   = -1;
		return(1);
		break;

	/* fstat() support code was written by Alain Magloire and slightly
	 * modified by Richard Dawe. Cheers Alain! */
	case __FSEXT_fstat:
		st = va_arg(args, struct stat *);
		/* most of the fields are irrelevent */
		bzero(st, sizeof(struct stat));
		st->st_mode |= S_IFSOCK;
		st->st_atime = st->st_ctime = st->st_mtime = time(0);
		*rv = 0;
		return(1);
		break;

	default:
		*rv = -1;
		return(1); /* That's better now */
    }

    /* This function hasn't handled the call */
    return(0);
}

/* ------------------
 * - __fd_is_socket -
 * ------------------ */

/* This checks whether or not the file descriptor refers to a socket. If the
 * file desciptor is a socket, then its File System Extensions handler will
 * be set to __lsck_socket_fsext() above. Hence, we check for that. */

int __fd_is_socket (int fd)
{
    __FSEXT_Function *f = __FSEXT_get_function (fd);

    if (f == NULL)
	return(0);
    if (f == __lsck_socket_fsext)
	return (1);
    else
	return(0);
}

/* -----------------
 * - __fd_is_valid -
 * ----------------- */

/* This function checks whether the file descriptor is valid. First, it checks
 * to see if the fd uses the File System Extensions. If not, it tries an
 * ioctl() that will succeed if fd is a normal file descriptor. */

int __fd_is_valid (int fd)
{
    int devdata;
    __FSEXT_Function *f = __FSEXT_get_function (fd);

    /* fd uses the File System Extension => valid. */
    if (f != NULL)
	return(1);

    /* fd is a normal file? This should work for all standard file
     * descriptors? */
    if (ioctl (fd, DOS_GETDEVDATA, 2, &devdata) != -1)
	return(1);

    /* Failed */
    return(0);
}

/* ------------
 * - isfdtype -
 * ------------ */

int isfdtype (int fd, int fd_type)
{
    /* Valid file descriptor? */
    if (!__fd_is_valid (fd)) {
	errno = EBADF;
	return(-1);
    }
    /* Socket? */
    if (fd_type == S_IFSOCK) {
	if (__fd_is_socket (fd))
	    return(1);
	else
	    return(0);
    }
    /* Some other fd query - unsupported */
    errno = EINVAL;
    return(-1);
}
