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

#ifndef _ARPA_INET_H
#define _ARPA_INET_H

#include <lsck/errno.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Fundamental Internet types */
#ifndef __libsocket_in_port_t
typedef unsigned short in_port_t;
#define __libsocket_in_port_t
#endif	/* __libsocket_in_port_t */

#ifndef __libsocket_in_addr_t
typedef unsigned in_addr_t;
#define __libsocket_in_addr_t
#endif	/* __libsocket_in_addr_t */

/* Internet address. */
#ifndef __libsocket_in_addr
#define __libsocket_in_addr
struct in_addr {
        in_addr_t s_addr;
};
#endif	/* __libsocket_in_addr */

/* Function declarations */
extern in_addr_t      inet_addr (const char *);
extern in_addr_t      inet_lnaof (struct in_addr);
extern struct in_addr inet_makeaddr (in_addr_t, in_addr_t);
extern in_addr_t      inet_netof (struct in_addr);
extern in_addr_t      inet_network (const char *);
extern char          *inet_ntoa (struct in_addr);
extern int            inet_aton (const char *, struct in_addr *);

/* New inet functions for handling IPv6 (and other) <-> text conversions. */
extern const char *inet_ntop (int , const void *, char *, size_t);
extern int inet_pton (int, const char *, void *);

/* NSAP address functions (que?) */
extern char *inet_nsap_ntoa (int,
			     register const unsigned char *,
			     register char *);

extern unsigned int inet_nsap_addr (const char *    /* ascii */,
				    unsigned char * /*binary */,
				    int             /* maxlen */);

#ifdef __cplusplus
}
#endif

#endif /* _ARPA_INET_H */
