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

#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

#ifndef __libsocket_socket_h__
#define __libsocket_socket_h__

#include <sys/types.h>
#include <sys/ioctl.h>
#include <lsck/bsdtypes.h>
#include <lsck/errno.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* --- Constants --- */

#define SOCK_STREAM     1               /* stream (connection) socket   */
#define SOCK_DGRAM      2               /* datagram (conn.less) socket  */
#define SOCK_RAW        3               /* raw socket                   */
#define SOCK_RDM        4               /* reliably-delivered message   */
#define SOCK_SEQPACKET  5

/* Flags we can use with send/ and recv. */
#define MSG_OOB		0x0001	/* Out-of-band data                        */
#define MSG_PEEK	0x0002	/* Leave received data in queue            */
#define MSG_DONTROUTE	0x0004	/* Send without using routing tables       */
#define MSG_CTRUNC	0x0008	/* Support this for BSD/Unix98 oddments    */
/* RD: Where did this come from? */
#define MSG_PROXY       0x0010	/* Supply or ask second address            */
#define MSG_EOR		0x0020	/* Terminate a record (if proto supports)  */
#define MSG_TRUNC   	0x0040	/* Normal data truncated                   */
#define MSG_WAITALL	0x0080	/* Wait for complete buffer                */
#define MSG_DONTWAIT	0x0100	/* Perform next operation without blocking */

/*
 * Maximum queue length specifiable by listen.
 */
#define SOMAXCONN       5

#define MSG_MAXIOVLEN   16
#define MSG_PARTIAL     0x8000	/* partial send or recv for message xport */

/* Setsockoptions(2) level. Thanks to BSD these must match IPPROTO_xxx */
#define SOL_IP          0
#define SOL_IPX         256
#define SOL_AX25        257
#define SOL_ATALK       258
#define SOL_NETROM      259
#define SOL_TCP         6
#define SOL_UDP         17

/* Link numbers.  */
#define IMPLINK_IP              155
#define IMPLINK_LOWEXPER        156
#define IMPLINK_HIGHEXPER       158

/*
 * Address families.
 */
#define AF_UNSPEC       0               /* unspecified */
#define AF_UNIX         1               /* local to host (pipes, portals) */
#define AF_INET         2               /* internetwork: UDP, TCP, etc. */
#define AF_IMPLINK      3               /* arpanet imp addresses */
#define AF_PUP          4               /* pup protocols: e.g. BSP */
#define AF_CHAOS        5               /* mit CHAOS protocols */
#define AF_IPX          6               /* IPX and SPX */
#define AF_NS           6               /* XEROX NS protocols */
#define AF_ISO          7               /* ISO protocols */
#define AF_OSI          AF_ISO          /* OSI is ISO */
#define AF_ECMA         8               /* european computer manufacturers */
#define AF_DATAKIT      9               /* datakit protocols */
#define AF_CCITT        10              /* CCITT protocols, X.25 etc */
#define AF_SNA          11              /* IBM SNA */
#define AF_DECnet       12              /* DECnet */
#define AF_DLI          13              /* Direct data link interface */
#define AF_LAT          14              /* LAT */
#define AF_HYLINK       15              /* NSC Hyperchannel */
#define AF_APPLETALK    16              /* AppleTalk */
#define AF_NETBIOS      17              /* NetBios-style addresses */
#define AF_VOICEVIEW    18              /* VoiceView */
#define AF_FIREFOX      19              /* FireFox */
#define AF_UNKNOWN1     20              /* Somebody is using this! */
#define AF_BAN          21              /* Banyan */

/* RD: From Winsock 2 headers, we have: */

/*#define AF_MAX          22*/

#define AF_ATM          22              /* Native ATM Services */
#define AF_INET6        23              /* Internetwork Ver. 6 (RD: IPv6?) */

#define AF_MAX          24

/* End RD */

/*
 * Protocol families, same as address families for now.
 */
#define PF_UNSPEC       AF_UNSPEC
#define PF_UNIX         AF_UNIX
#define PF_INET         AF_INET
#define PF_IMPLINK      AF_IMPLINK
#define PF_PUP          AF_PUP
#define PF_CHAOS        AF_CHAOS
#define PF_NS           AF_NS
#define PF_IPX          AF_IPX
#define PF_ISO          AF_ISO
#define PF_OSI          AF_OSI
#define PF_ECMA         AF_ECMA
#define PF_DATAKIT      AF_DATAKIT
#define PF_CCITT        AF_CCITT
#define PF_SNA          AF_SNA
#define PF_DECnet       AF_DECnet
#define PF_DLI          AF_DLI
#define PF_LAT          AF_LAT
#define PF_HYLINK       AF_HYLINK
#define PF_APPLETALK    AF_APPLETALK
#define PF_VOICEVIEW    AF_VOICEVIEW
#define PF_FIREFOX      AF_FIREFOX
#define PF_UNKNOWN1     AF_UNKNOWN1
#define PF_BAN          AF_BAN

/* RD: Winsock 2 new ones (see AF_* above) */

#define PF_ATM          AF_ATM
#define PF_INET6        AF_INET6

/* End RD */

#define PF_MAX          AF_MAX

/*
 * Option flags per-socket.
 */
#define SO_DEBUG        0x0001          /* turn on debugging info recording */
#define SO_ACCEPTCONN   0x0002          /* socket has had listen() */
#define SO_REUSEADDR    0x0004          /* allow local address reuse */
#define SO_KEEPALIVE    0x0008          /* keep connections alive */
#define SO_DONTROUTE    0x0010          /* just use interface addresses */
#define SO_BROADCAST    0x0020          /* permit sending of broadcast msgs */
#define SO_USELOOPBACK  0x0040          /* bypass hardware when possible */
#define SO_LINGER       0x0080          /* linger on close if data present */
#define SO_OOBINLINE    0x0100          /* leave received OOB data in line */

#define SO_DONTLINGER   (u_int)(~SO_LINGER)

/*
 * Additional options.
 */
#define SO_SNDBUF       0x1001          /* send buffer size */
#define SO_RCVBUF       0x1002          /* receive buffer size */
#define SO_SNDLOWAT     0x1003          /* send low-water mark */
#define SO_RCVLOWAT     0x1004          /* receive low-water mark */
#define SO_SNDTIMEO     0x1005          /* send timeout */
#define SO_RCVTIMEO     0x1006          /* receive timeout */
#define SO_ERROR        0x1007          /* get error status and clear */
#define SO_TYPE         0x1008          /* get socket type */

/*
 * Options for connect and disconnect data and options.  Used only by
 * non-TCP/IP transports such as DECNet, OSI TP4, etc.
 */
#define SO_CONNDATA     0x7000
#define SO_CONNOPT      0x7001
#define SO_DISCDATA     0x7002
#define SO_DISCOPT      0x7003
#define SO_CONNDATALEN  0x7004
#define SO_CONNOPTLEN   0x7005
#define SO_DISCDATALEN  0x7006
#define SO_DISCOPTLEN   0x7007

/*
 * Option for opening sockets for synchronous access.
 */
#define SO_OPENTYPE     0x7008

#define SO_SYNCHRONOUS_ALERT    0x10
#define SO_SYNCHRONOUS_NONALERT 0x20

/*
 * Other NT-specific options.
 */
#define SO_MAXDG        0x7009
#define SO_MAXPATHDG    0x700A
#define SO_UPDATE_ACCEPT_CONTEXT 0x700B
#define SO_CONNECT_TIME 0x700C

/*
 * TCP options.
 */
/*#define TCP_NODELAY     0x0001*/
#define TCP_BSDURGENT   0x7000

#define SOL_SOCKET      0xffff          /* options for socket level */

/*
 * Define constant based on rfc883, used by gethostbyxxxx() calls.
 */
#define MAXGETHOSTSTRUCT        1024

/* ioctl() and fcntl() defines. Currently #include <sys/ioctl.h> doesn't
   define the required constants because of an #ifdef. So, define them here.
   This may lead to problems later :( */

#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif

#ifndef FNDELAY
#define FNDELAY O_NONBLOCK
#endif

#ifndef FIONREAD
#define FIONREAD    _IOR('f', 127, int)    /* get # bytes to read */
#endif

#ifndef FIONBIO
#define FIONBIO     _IOW('f', 126, int)    /* set/clear non-blocking i/o */
#endif

#ifndef FIOASYNC
#define FIOASYNC    _IOW('f', 125, int)    /* set/clear async i/o */
#endif

/* Socket I/O Controls */
#define SIOCSHIWAT  _IOW('s',  0, int)     /* set high watermark */
#define SIOCGHIWAT  _IOR('s',  1, int)     /* get high watermark */
#define SIOCSLOWAT  _IOW('s',  2, int)     /* set low watermark */
#define SIOCGLOWAT  _IOR('s',  3, int)     /* get low watermark */
#define SIOCATMARK  _IOR('s',  7, int)     /* at oob mark? */

/*
 * Define flags to be used with the WSAAsyncSelect() call.
 */
/* RD: To use some of these requires Dan Hedlund's callback code IMHO. */
#define FD_READ         0x01
#define FD_WRITE        0x02
#define FD_OOB          0x04
#define FD_ACCEPT       0x08
#define FD_CONNECT      0x10
#define FD_CLOSE        0x20

/* how parameter of shutdown */
#define SHUT_RD		0
#define SHUT_WR		1
#define SHUT_RDWR	2

/* --- Types --- */

#ifndef __libsocket_socklen_t
typedef unsigned socklen_t;
#define __libsocket_socklen_t
#endif	/* __libsocket_socklen_t */

#ifndef __libsocket_sa_family_t
typedef unsigned short sa_family_t;
#define __libsocket_sa_family_t
#endif	/* __libsocket_sa_family_t */

/* - Structures - */

struct sockaddr
{
	sa_family_t sa_family; /* address family   */
	char        sa_data[14]; /* protocol address */
};

struct linger {
        unsigned short l_onoff; /* Linger active   */
        unsigned short l_linger; /* Linger interval */
};

/* Support for readmsg(), sendmsg(). */
struct msghdr {
	void         *msg_name;		/* Optional address             */
	socklen_t     msg_namelen;	/* Size of address              */
	struct iovec *msg_iov;		/* Scatter/gather array         */
	int           msg_iovlen;	/* # of members in msg_iov      */
	void         *msg_control;	/* Ancillary data               */
	socklen_t     msg_controllen;	/* Ancillary data buffer length */
	int           flags;		/* Flags on received message    */
};

/* Ancillary data type */
struct cmsghdr {
	socklen_t cmsg_len;	/* Data byte count, including the cmsghdr */
	int       cmsg_level;	/* Originating protocol                   */
	int       cmsg_type;	/* Protocol-specific type                 */
};

/* TODO: Implement the following:

   . SCM_RIGHTS
   . CMSG_DATA
   . CMSG_NXTHDR
   . CMSG_FIRSTHDR
*/

/* --- Functions --- */

extern int socket (int, int, int);
extern int socketpair (int, int, int, int [2]);
extern int bind (int, struct sockaddr *, size_t);
extern int listen (int, int);
extern int connect (int, struct sockaddr *, size_t);
extern int accept (int, struct sockaddr *, size_t *);

extern ssize_t recv (int, void *, size_t, unsigned int);
extern ssize_t recvfrom (int, void *, size_t, unsigned int,
                         struct sockaddr *, size_t *);
extern ssize_t recvmsg (int, struct msghdr *, int);

extern ssize_t send (int, const void *, size_t, unsigned int);
extern ssize_t sendto (int, const void *, size_t, unsigned int,
		       struct sockaddr *, size_t);
extern ssize_t sendmsg (int, const struct msghdr *, int);

extern int getsockname (int, struct sockaddr *, size_t *);
extern int getpeername (int, struct sockaddr *, size_t *);
extern int getsockopt (int, int, int, void *, size_t *);
extern int setsockopt (int, int, int, const void *, size_t);

extern int shutdown (int, int);

extern int rcmd (char **, unsigned short, const char *,
                 const char *, const char *, int *);
extern int rresvport (int *);
extern int ruserok (const char *, int, const char *, const char *);
extern int rexec (char **, int, const char *, const char *, const char *,
		  int *);

extern int sockatmark (int);

/* Non-standard socket calls */
extern int closesocket (int);
extern int ioctlsocket (int, int, int *);
extern int fcntlsocket (int, int, int);
extern int selectsocket (int, int);

/* --- Out-of-place defines, functions, etc. --- */

/* - sys/stat.h - */
extern int isfdtype (int, int);

#ifndef S_IFSOCK
#define S_IFSOCK 0xC000
#endif

#ifndef S_ISSOCK
#define S_ISSOCK(mode) ((mode & 0xF000) == S_IFSOCK)
#endif

#ifdef __cplusplus
}
#endif

#endif  /* __libsocket_socket_h__ */

#endif	/* _SYS_SOCKET_H */
