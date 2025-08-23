#ifndef CSOCK_H
#define CSOCK_H

#include <libc/asmobj.h>
#include <sys/cdefs.h>
#include <lsck/lsck.h>

LONG32 ___csock_get_version(__out ULONG32 *ver, ULONG32 len, __out struct driver_info_rec *di);
LONG32 ___csock_open(ULONG32 proto, __out ULONG32 *fd);
LONG32 ___csock_close(ULONG32 fd);
LONG32 ___csock_bind(ULONG32 fd, ULONG32 addr, unsigned short port);
LONG32 ___csock_sendto(ULONG32 fd, ULONG32 addr, unsigned short port, const void _V_FW(1) *buf, ULONG32 len, __out ULONG32 *sent);
LONG32 ___csock_send(ULONG32 fd, const void _V_FW(1) *buf, ULONG32 len, __out ULONG32 *sent);
LONG32 ___csock_recvfrom(ULONG32 fd, __out void _V_FW(1) *buf, ULONG32 len, __out ULONG32 *rcvd, __out ULONG32 *addr, __out unsigned short *port);
LONG32 ___csock_recv(ULONG32 fd, __out void _V_FW(1) *buf, ULONG32 len, __out ULONG32 *rcvd);
LONG32 ___csock_listen(ULONG32 fd, ULONG32 backlog);
LONG32 ___csock_accept(ULONG32 fd, __out ULONG32 *afd, __out ULONG32 *addr, __out unsigned short *port);
LONG32 ___csock_select(ULONG32 *rfds, ULONG32 *wfds, ULONG32 *efds, ULONG32 *timeout);
LONG32 ___csock_connect(ULONG32 fd, ULONG32 addr, unsigned short port);
LONG32 ___csock_getsockname(ULONG32 fd, __out ULONG32 *addr, __out unsigned short *port);
LONG32 ___csock_getpeername(ULONG32 fd, __out ULONG32 *addr, __out unsigned short *port);
LONG32 ___csock_getnblkio(ULONG32 fd, __out ULONG32 *nb);
LONG32 ___csock_setnblkio(ULONG32 fd, ULONG32 nb);
LONG32 ___csock_fionread(ULONG32 fd, __out ULONG32 *outq);
LONG32 ___csock_getsoerr(ULONG32 fd, __out ULONG32 *outerr);
LONG32 ___csock_setsolinger(ULONG32 fd, ULONG32 on, ULONG32 linger);

#endif
