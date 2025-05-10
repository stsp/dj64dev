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
   resolv.h - resolv include file.

   Created by Richard Dawe, based on the format of the RedHat Linux 5.1
   resolv.h file. Made resolver functions externs.
*/

#ifndef __libsocket_resolv_h__
#define __libsocket_resolv_h__

#include <arpa/nameser.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Resolver options (keep these in synch with res_debug.c, please)
 */

#define RES_INIT        0x00000001      /* address initialized */
#define RES_DEBUG       0x00000002      /* print debug messages */
#define RES_AAONLY      0x00000004      /* authoritative answers only (!IMPL)*/
#define RES_USEVC       0x00000008      /* use virtual circuit */
#define RES_PRIMARY     0x00000010      /* query primary server only (!IMPL) */
#define RES_IGNTC       0x00000020      /* ignore trucation errors */
#define RES_RECURSE     0x00000040      /* recursion desired */
#define RES_DEFNAMES    0x00000080      /* use default domain name */
#define RES_STAYOPEN    0x00000100      /* Keep TCP socket open */
#define RES_DNSRCH      0x00000200      /* search up local domain tree */
#define RES_INSECURE1   0x00000400      /* type 1 security disabled */
#define RES_INSECURE2   0x00000800      /* type 2 security disabled */
#define RES_NOALIASES   0x00001000      /* shuts off HOSTALIASES feature */

#define RES_DEFAULT     (RES_RECURSE | RES_DEFNAMES | RES_DNSRCH)

/*
 * Resolver "pfcode" values.  Used by dig.
 */
#define RES_PRF_STATS   0x00000001
/*                      0x00000002      */
#define RES_PRF_CLASS   0x00000004
#define RES_PRF_CMD     0x00000008
#define RES_PRF_QUES    0x00000010
#define RES_PRF_ANS     0x00000020
#define RES_PRF_AUTH    0x00000040
#define RES_PRF_ADD     0x00000080
#define RES_PRF_HEAD1   0x00000100
#define RES_PRF_HEAD2   0x00000200
#define RES_PRF_TTLID   0x00000400
#define RES_PRF_HEADX   0x00000800
#define RES_PRF_QUERY   0x00001000
#define RES_PRF_REPLY   0x00002000
#define RES_PRF_INIT    0x00004000
/*                      0x00008000      */

/* hooks are still experimental as of 4.9.2 */
typedef enum { res_goahead, res_nextns, res_modified, res_done, res_error }
        res_sendhookact;

/*
 * Global defines and variables for resolver stub.
 */
#define MAXNS                   3       /* max # name servers we'll track */
#define MAXDFLSRCH              3       /* # default domain levels to try */
#define MAXDNSRCH               6       /* max # domains in search path */
#define LOCALDOMAINPARTS        2       /* min levels in name that is "local" */
#define RES_TIMEOUT             5       /* min. seconds between retries */
#define MAXRESOLVSORT           10      /* number of net to sort on */
#define RES_MAXNDOTS            15      /* should reflect bit field size */

#define MAXHOSTNAMELEN  64      /* max length of hostname */

struct __res_state {
        int     retrans;                /* retransmition time interval */
        int     retry;                  /* number of times to retransmit */
        unsigned long  options;                /* option flags - see below. */
        int     nscount;                /* number of name servers */
        struct sockaddr_in
                nsaddr_list[MAXNS];     /* address of name server */
#define nsaddr  nsaddr_list[0]          /* for backward compatibility */
        unsigned short id;                     /* current packet id */
        char    *dnsrch[MAXDNSRCH+1];   /* components of domain to search */
        char    defdname[MAXDNAME];     /* default domain */
        unsigned long  pfcode;                 /* RES_PRF_ flags - see below. */
        unsigned ndots:4;               /* threshold for initial abs. query */
        unsigned nsort:4;               /* number of elements in sort_list[] */
        char    unused[3];
        struct {
                struct in_addr  addr;
                unsigned long int       mask;
        } sort_list[MAXRESOLVSORT];
};

extern struct __res_state _res;

extern int h_errno;

extern int res_query ( const char *, int, int, unsigned char *, int );
extern int res_search ( const char *, int, int, unsigned char *, int );
extern int res_mkquery ( int, const char *, int, int, const unsigned char *, int, const unsigned char *, unsigned char *, int);
extern int res_send ( const unsigned char *, int, unsigned char *, int );
extern int res_init ( void );
extern int dn_comp ( const char *, unsigned char *, int, unsigned char **, unsigned char **);
extern int dn_expand ( const unsigned char *, const unsigned char *, const unsigned char *, char *, int );

#ifdef __cplusplus
}
#endif

#endif  /* __libsocket_resolv_h__ */
