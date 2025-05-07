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

/* addr.c - Common interface functions */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <lsck/lsck.h>
#include <lsck/ini.h>
#include <lsck/if.h>

/* --------------------------------
 * - __lsck_if_addr_inet_loopback -
 * -------------------------------- */

/* This builds and returns an interface Internet address data structure for
 * the loopback device. This is used in win3x/w3x_addr.c and win9x/w9x_addr.c,
 * but could be used by any interface. */

LSCK_IF_ADDR_INET *__lsck_if_addr_inet_loopback (void)
{
    LSCK_IF_ADDR_INET *lo = NULL;

    lo = (LSCK_IF_ADDR_INET *) malloc (sizeof (LSCK_IF_ADDR_INET));
    if (lo == NULL)
	return (NULL);

    bzero (lo, sizeof (LSCK_IF_ADDR_INET));
    lo->addr.s_addr = htonl (INADDR_LOOPBACK);
    lo->netmask.s_addr = htonl (IN_CLASSA_NET);
    lo->gw_present = 0;
    lo->fixed = 1;
    lo->hw_type = LSCK_IF_HW_TYPE_VIRTUAL;

    return (lo);
}

/* ---------------------------------
 * - __lsck_if_addr_inet_getconfig -
 * --------------------------------- */

LSCK_IF_ADDR_INET *__lsck_if_addr_inet_getconfig (const char *section)
{
    LSCK_IF_ADDR_INET *inet = NULL;
    char *cfg = NULL;
    char addrbuf[16];		/* Dotted-quad with nul */
    char dnskey[16];
    char hostname[MAXGETHOSTNAME];
    char *p = NULL;
    int i, ret;

    /* Initialise */
    inet = (LSCK_IF_ADDR_INET *) malloc (sizeof (LSCK_IF_ADDR_INET));
    if (inet == NULL) return (NULL);
    bzero (inet, sizeof (LSCK_IF_ADDR_INET));

    cfg = __lsck_config_getfile();
    if (cfg == NULL) return (NULL);

#define BAD_IP      "0.0.0.0"

    /* IP address */
    GetPrivateProfileString (section, "IPAddress", BAD_IP,
			     addrbuf, sizeof (addrbuf), cfg);
    ret = (strcmp (addrbuf, BAD_IP) == 0);

    if (!ret)
	ret = (inet_aton (addrbuf, &inet->addr) == 0);

    if (ret) {
	free (inet);
	return (NULL);
    }
    /* Network mask */
    GetPrivateProfileString (section, "IPMask", BAD_IP,
			     addrbuf, sizeof (addrbuf), cfg);
    ret = (strcmp (addrbuf, BAD_IP) == 0);

    if (!ret)
	ret = (inet_aton (addrbuf, &inet->netmask) == 0);

    if (ret) {
	free (inet);
	return (NULL);
    }
    /* Gateway */
    GetPrivateProfileString (section, "Gateway", BAD_IP,
			     addrbuf, sizeof (addrbuf), cfg);
    if ((strcmp (addrbuf, BAD_IP) != 0)
	&& (inet_aton (addrbuf, &inet->gw) != 0)) {
	inet->gw_present = 1;
    }
    /* DNS */
    for (i = 0; i < LSCK_IF_ADDR_INET_DNS_MAX; i++) {
	sprintf (dnskey, "DNS%iAddress", i + 1);
	GetPrivateProfileString (section, dnskey, BAD_IP,
				 addrbuf, sizeof (addrbuf), cfg);

	if (strcmp (addrbuf, BAD_IP) == 0)
	    break;
	if (inet_aton (addrbuf, &inet->dns[i]) == 0)
	    break;
    }

    /* Hardware */
    inet->hw_type = LSCK_IF_HW_TYPE_ETHERNET;
    inet->hw_addrlen = LSCK_IF_HW_ADDRLEN_ETHERNET;

    /* Host and domain name */
    GetPrivateProfileString ("main", "hostname", NULL,
			     hostname, sizeof (hostname), cfg);

    if (*hostname != '\0') {
	p = strchr (hostname, '.');
	if (p != NULL) {
	    *p = '\0';
	    p++;
	}
	strcpy (inet->hostname, hostname);
	strcpy (inet->domainname, p);
    }
    /* Done */
    return (inet);
}
