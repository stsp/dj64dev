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

/* GNU indent seems to have trouble with this file :( */
/* *INDENT-OFF* */

#include <stdlib.h>

#include <dpmi.h>

#include <lsck/if.h>
#include <lsck/csock.h>
#include "csock.h"

/* ------------------
   - __csock_accept -
   ------------------ */

int
__csock_accept (LSCK_SOCKET *lsd, LSCK_SOCKET *nsd,
		struct sockaddr *addr, size_t *addrlen)
{
  LSCK_SOCKET_CSOCK *csock    = (LSCK_SOCKET_CSOCK *) lsd->idata;
  LSCK_SOCKET_CSOCK *ncsock   = NULL;
  int ret                     = -1;
  unsigned short dest_port    =  0;
  unsigned int dest_addr      =  0;
  struct sockaddr_in *addr_sa = (struct sockaddr_in *) addr;
  int blocking                = lsd->blocking;
  int flag                    = lsd->blocking;
  unsigned int i;
  int rv;

  /* Check there's enough storage space */
  if (*addrlen < sizeof(struct sockaddr_in)) {
    errno = EINVAL;
    return(-1);
  }

  /* Allocate a buffer for the interface data. */
  ncsock = (LSCK_SOCKET_CSOCK *) malloc(sizeof(*ncsock));
  nsd->idata = (void *) ncsock;

  if (csock == NULL) {
    errno = ENOMEM;
    return(-1);
  }

  /* If the socket is in blocking mode, flip into non-blocking mode.
   * to allow interruption with Ctrl+C. lsd->blocking is saved in blocking,
   * because the ioctl alters lsd->blocking. */
  if (blocking) {
    flag = blocking;
    __csock_ioctl(lsd, &rv, FIONBIO, &flag);
  }

tryitagain:
    ret = ___csock_accept(csock->fd, &i, &dest_addr, &dest_port);
    /* No connection this time, so try again. */
    if (blocking && (ret == -CSOCK_ERR_WOULD_BLOCK)) {
      /* Let someone else do something */
      __dpmi_yield();
      goto tryitagain;
    }

    /* Got one! */
    if (ret == 0) {
      ncsock->fd = i;
    }

  /* Switch back to blocking mode, if it was in it originally. */
  if (blocking) {
    flag = !blocking;
    __csock_ioctl(lsd, &rv, FIONBIO, &flag);
  }

  if (ret != 0) {
    errno = __csock_errno(ret);
    return(-1);
  }

  /* Copy the peer address */
  addr_sa->sin_family      = AF_INET;
  addr_sa->sin_addr.s_addr = dest_addr;
  addr_sa->sin_port        = dest_port;
  *addrlen                 = sizeof(struct sockaddr_in);

  /* Mark the fd as used */
//  __csock_fd_set_used(ncsock->fd);

  return(0);
}
