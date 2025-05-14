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

#include <net/if.h>

#include <lsck/if.h>
#include <lsck/errno.h>
#include <lsck/lsck.h>

extern LSCK_IF *__lsck_interface[LSCK_MAX_IF + 1];
static struct if_nameindex __lsck_if_nameindex[LSCK_MAX_IF + 1];

/* ----------------------
 * - build_if_nameindex -
 * ---------------------- */

static int build_if_nameindex (void)
{
    static int built_if_nameindex = 0;
    int i, count;

    if (!__lsck_init ()) {
	/* TODO: Not sure which error to return here. */
	errno = ENODEV;
	/*errno = EPROTONOSUPPORT; */
	return (-1);
    }

    /* Already done it */
    if (built_if_nameindex)
	return (built_if_nameindex);

    /* Use the existing interface data */
    for (count = i = 0; i <= LSCK_MAX_IF; i++) {
	if (__lsck_interface[i] == NULL)
	    continue;

	__lsck_if_nameindex[count].if_index = count + 1;
	__lsck_if_nameindex[count].if_name = __lsck_interface[i]->name;
	count++;
    }

    __lsck_if_nameindex[count].if_index = 0;
    __lsck_if_nameindex[count].if_name = NULL;

    built_if_nameindex = 1;
    return (built_if_nameindex);
}

/* ------------------
 * - if_nametoindex -
 * ------------------ */

unsigned int if_nametoindex (const char *ifname)
{
    int i;

    /* Valid param? */
    if (ifname == NULL) {
        errno = EFAULT;
	return(-1); /* TODO: Is this correct */
    }

    /* Build the interface name-index list */
    if (!build_if_nameindex ())
	return (0);

    for (i = 0; __lsck_if_nameindex[i].if_name != NULL; i++) {
	if (strcmp (__lsck_if_nameindex[i].if_name, ifname) == 0)
	    return (__lsck_if_nameindex[i].if_index);
    }

    /* Not found */
    return (0);
}

/* ------------------
 * - if_indextoname -
 * ------------------ */

char *if_indextoname (unsigned int ifindex, char *ifname)
{
    int i;

    /* Unable to get the interface list */
    if (!build_if_nameindex ()) {
	errno = ENXIO;
	return (NULL);
    }
    /* Find the interface */
    for (i = 0; __lsck_if_nameindex[i].if_index != 0; i++) {
	if (__lsck_if_nameindex[i].if_index == ifindex) {
	    strcpy (ifname, __lsck_if_nameindex[i].if_name);
	    return (ifname);
	}
    }

    /* No match */
    errno = ENXIO;
    return (NULL);
}

/* ----------------
 * - if_nameindex -
 * ---------------- */

struct if_nameindex *if_nameindex (void)
{
    if (!build_if_nameindex ()) {
	errno = ENOBUFS;
	return (NULL);
    }
    return (__lsck_if_nameindex);
}

/* --------------------
 * - if_freenameindex -
 * -------------------- */

/* This doesn't actually do anything. Since no memory was allocated to
 * perform the if_nameindex() call, a free isn't needed. */

void if_freenameindex (struct if_nameindex *ptr)
{;
}
