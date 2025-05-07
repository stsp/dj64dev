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
#include <stdlib.h>
#include <string.h>

#include <dpmi.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>		       /* For IFNAMSIZ def */

#include <lsck/windows.h>
#include <lsck/if.h>
#include <lsck/csock.h>
#include "csock.h"

#include <lsck/lsck.h>
#include <lsck/ini.h>
#include <lsck/win9x.h>
#include <lsck/win3x.h>

#include <lsck/vxd.h>

/* --- Global variables --- */

char CSOCK_IF_NAME[IFNAMSIZ] = "Coda SOCK.VXD";

LSCK_IF __csock_interface =
{
	/* Interface information */
	CSOCK_IF_NAME,
	IF_FLAG_HAS_NONBLOCKING,
	(LSCK_IF_ADDR_TABLE *) NULL,
	(LSCK_IF_ROUTE_TABLE *) NULL,

	/* Interface management functions */
	NULL, __csock_init, NULL, __csock_uninit,

	/* General functions */
	__csock_socket, __csock_socketpair, __csock_close,
	__csock_proto_check, __csock_addrlen_check,
	__csock_bind, __csock_listen, __csock_accept,
	__csock_connect,
	__csock_recv, __csock_recvfrom,
	__csock_send, __csock_sendto,
	__csock_getsockname, __csock_getpeername,
	__csock_getsockopt, NULL,
	__csock_shutdown,

	/* I/O control functions */
	__csock_fcntl, __csock_ioctl, __csock_select,
	__csock_nonblocking_check,
	NULL /* sockatmark */
};

static int CSOCK_VXD_ID = 0x1235;
//static char CSOCK_VXD_NAME[] = "SOCK.VXD";
//int csock_entry[2];
unsigned int __csock_version = 0;

static int csock_inited = 0;

/* IP information */
#define DHCP_IP "0.0.0.0"

/* ----------------
 * - __csock_init -
 * ---------------- */

LSCK_IF *__csock_init (void)
{
	LSCK_IF_ADDR_INET *inet = NULL;
	char *cfg = NULL;
	char enabled_buf[6];	/* 5 letters/digits + nul = 5 bytes */
	int win_version, ret;

	if (csock_inited)
		return (&__csock_interface);

	/* Check that this interface is enabled. If the "Enabled" entry is
	 * present in the configuration file, presume that it's enabled unless
	 * one of the values false, no or 0 is used. */
	cfg = __lsck_config_getfile();

	if (cfg != NULL)
		GetPrivateProfileString ("csock", "Enabled", "true",
					 enabled_buf, sizeof (enabled_buf),
					 cfg);
	else
		strcpy(enabled_buf, "true");

	if ((stricmp (enabled_buf, "false") == 0)
	    || (stricmp (enabled_buf, "no") == 0)
	    || (strcmp (enabled_buf, "0") == 0)) {
		if (__lsck_debug_enabled ())
			printf ("csock: Interface disabled\n");
		return (NULL);
	}

	win_version = __get_windows_version();

	if (__lsck_debug_enabled ()) {
		switch (win_version) {
		case WIN_311:
			/* TODO: Does SOCK.VXD work with Windows 3.x? */
			printf("csock: Windows 3.x detected\n");
			break;

		case WIN_95:
			printf("csock: Windows '95 (& later) detected\n");
			break;

		case WIN_98:
			printf("csock: Windows '98 detected\n");
			break;

		case WIN_NT:
			printf("csock: Windows NT detected\n");
			break;

		case WIN_NONE:
		default:
			printf("csock: Windows not detected\n");
			break;
		}
	}

	/* No Windows => no Winsock => fail! */
	switch(win_version) {
	/* Unsupported */
	case WIN_NONE:
	case WIN_NT:
	    return (NULL);
	    break;

	default:
	    break;
	}

	/* Load the VxD and get its entry point */
//	if (VxdLdrLoadDevice (CSOCK_VXD_NAME) != 0)
//		return (NULL);

//	csock_entry[0] = csock_entry[1] = 0;
	ret = VxdGetEntry (/*csock_entry, */CSOCK_VXD_ID);
	if (ret < 0) {
		errno = ENODEV;
		return (NULL);
	}

	/* Get the VxD version - This is a little tricky as the original
	 * versions of SOCK.VXD do not support this function call (EAX = 0).
	 * They will try to find the socket descriptor specified by EDI, so
	 * pass a bogus one to ensure failure. */
	ret = ___csock_get_version(&__csock_version);
	if (ret < 0)
		return NULL;

	/* - Get the IP details - */

	/* Config override? */
	inet = __lsck_if_addr_inet_getconfig ("csock");

	if (inet != NULL) {
		/* If malloc fails, fall back on auto */
		__csock_interface.addr =
			(LSCK_IF_ADDR_TABLE *)
			malloc (sizeof (LSCK_IF_ADDR_TABLE));

		if (__csock_interface.addr != NULL) {
			__csock_interface.addr->family = AF_INET;
			__csock_interface.addr->table.inet[0] =
				__lsck_if_addr_inet_loopback ();
			__csock_interface.addr->table.inet[1] = inet;
			__csock_interface.addr->table.inet[2] = NULL;
		}
	}

	/* No config override, so try to configure automatically */
//	if (__csock_interface.addr == NULL)
//		__csock_interface.addr = __win9x_getaddrtable ();

	if (__csock_interface.addr == NULL) {
		csock_inited = 1;
		__csock_uninit ();
		return (NULL);
	}

	/* Done */
	csock_inited = 1;
	return (&__csock_interface);
}

/* ------------------
 * - __csock_uninit -
 * ------------------ */

int __csock_uninit (void)
{
	if (!csock_inited) return (1);

	/* Unloading the VxD may cause protection faults in Windows's Virtual
	 * Memory Manager (VMM). Maybe SOCK.VXD doesn't clean up properly when
	 * it's unloaded or a socket is closed (i.e. TDI still thinks it's
	 * using the memory, tries to access it -> PF). */

	/*VxdLdrUnLoadDevice ("SOCK.VXD");*/

	csock_inited = 0;
	return (1);
}

/* -----------------------
 * - __csock_proto_check -
 * -----------------------  */

/* sock.vxd supported protocols. This was found by looking at sock.vxd's
 * source. */

int __csock_proto_check (int domain, int type, int protocol)
{
	/* sock.vxd support inited? */
	if (!csock_inited) return (0);

	/* Only Internet sockets */
	if (domain != AF_INET) return (0);

	/* Only TCP streams, UDP datagrams */
	if (((type == SOCK_STREAM) && (protocol == IPPROTO_TCP))
	    || ((type == SOCK_DGRAM) && (protocol == IPPROTO_UDP)))
		return (1);

	/* Fail all others */
	return (0);
}

/* -------------------------
 * - __csock_addrlen_check -
 * ------------------------- */

int __csock_addrlen_check (LSCK_SOCKET *lsd, size_t addrlen)
{
	if (lsd->family != AF_INET) return(0);
	if ((lsd->type != SOCK_STREAM) && (lsd->type != SOCK_DGRAM)) return(0);
	if (addrlen < sizeof(struct sockaddr_in)) return(0);
	return(1);
}
