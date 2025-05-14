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
 * dnsaddr.c
 *
 * This code obtains the DNS server's IP address from the interface data
 * and resolv.conf.
 */

/* Include files */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <resolv.h>

#include <lsck/if.h>
#include <lsck/lsck.h>
#include <lsck/ini.h>

/* Globals */
extern LSCK_IF *__lsck_interface[LSCK_MAX_IF + 1];

/* ----------------------------------
 * - __lsck_resolv_conf_getdnsaddrs -
 * ---------------------------------- */

/* This reads the DNS addresses from resolv.conf and places them in a
 * NULL-terminated list. */

static char **__lsck_resolv_conf_getdnsaddrs (void)
{
	struct in_addr testaddr;
	char   buf[256];
	char   resolv_conf[PATH_MAX];
	char  *cfg      = __lsck_config_getfile();
	char **dnsaddrs = NULL;
	FILE  *fp       = NULL;
	char  *p        = NULL;
	char  *q        = NULL;
	int    count    = 0;
	int    i;

	/* Find resolv.conf */
	if (cfg == NULL) return(NULL);
	GetPrivateProfileString("main", "resolv.conf", NULL,
	                        resolv_conf, sizeof(resolv_conf), cfg);

	fp = fopen(resolv_conf, "rt");
	if (fp == NULL) return(NULL);

	/* Allocate memory - max is MAXNS domain names in resolv.conf */
	dnsaddrs = (char **) malloc((MAXNS + 1) * sizeof(char *));
	if (dnsaddrs == NULL) {
		/* TODO: Debug message? */
		fclose(fp);
		return(NULL);
	}

	for (i = 0; i <= MAXNS; i++) { dnsaddrs[i] = NULL; }

	/* Parse the file to find nameserver keys. */
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		p = buf;
		while (isspace(*p)) { p++; }
		if (*p == '#') continue;

		/* nameserver line? Only one IP address is allowed per
		 * line. */
		if (strncmp(p, "nameserver", strlen("nameserver")) != 0)
			continue;

		p += strlen("nameserver");
		if (!isspace(*p)) continue;

		/* Skip all the spaces */
		while (isspace(*p)) { p++; }

		/* Nuke non-numeric or dots characters in the address. */
		q = p;
		while (isdigit(*q) || (*q == '.')) { q++; }
		*q = '\0';

		/* Is this a valid address? */
		if (inet_aton(p, &testaddr) == 0) continue;
		dnsaddrs[count] = strdup(p);
	}

	fclose(fp);
	return(dnsaddrs);
}

/* ---------------------
 * - __lsck_getdnsaddr -
 * --------------------- */

char *__lsck_getdnsaddr (void)
{
    char **dnsaddrs = NULL, *dnsaddr = NULL;
    char **p;

    /* Get all addresses, but only return first */
    p = dnsaddrs = __lsck_getdnsaddrs ();

    if (dnsaddrs == NULL)
	    return (NULL);

    if (*dnsaddrs == NULL) {
	    free(dnsaddrs);
	    return(NULL);
    }

    /* This is the only one we want */
    dnsaddr = *dnsaddrs;

    /* Free all the unused memory */
    for (p++; *p != NULL; p++) { free (*p); }
    free (dnsaddrs);

    /* Return the IP address */
    return (dnsaddr);
}

/* -------------
 * - strinlist -
 * ------------- */

/* This function checks to see if the string 'str' is anywhere in 'list', e.g.
 * for finding duplicate entries. */
#if 0
int strinlist (char **list, char *str)
{
    int i;

    for (i = 0; list[i] != NULL; i++) {
	    /* Found match */
	    if (strcmp (list[i], str) == 0)
		    return (1);
    }

    /* No match */
    return (0);
}
#endif
/* ----------------------
 * - __lsck_getdnsaddrs -
 * ---------------------- */

/* This goes through the list of interfaces extracting DNS IP data and
 * compiles them into a list. The DNS's are stored in order of occurrence. */

char **__lsck_getdnsaddrs (void)
{
    LSCK_IF *interface = NULL;
    LSCK_IF_ADDR_INET *inet = NULL;
    char **resolv_conf_dnsaddrs = NULL;
    char **dnsaddrs = NULL;
    int i, j, k, count;

    if (!__lsck_init ()) {
	/* TODO: Not sure which error to return here. */
	errno = ENODEV;
	/*errno = EPROTONOSUPPORT; */
	return NULL;
    }

    /* Get all the DNS addresses. */
    count = 0;

    resolv_conf_dnsaddrs = __lsck_resolv_conf_getdnsaddrs();
    if (resolv_conf_dnsaddrs != NULL) {
	    for (i = 0; resolv_conf_dnsaddrs[i] != NULL; count++, i++) { ; }
    }

    for (i = 0; (interface = __lsck_interface[i]) != NULL; i++) {
	    if (interface->addr == NULL)
		    continue;
	    if (interface->addr->family != AF_INET)
		    continue;

	    for (j = 0; (inet = interface->addr->table.inet[j]) != NULL; j++) {
		    for (k = 0; k < LSCK_IF_ADDR_INET_DNS_MAX; k++) {
			    if (inet->dns[k].s_addr == 0)
				    break;
			    count++;
		    }
	    }
    }

    dnsaddrs = malloc (sizeof (char *) * (count + 1));
    if (dnsaddrs == NULL)
	    return (NULL);

    for (i = 0; i < count + 1; i++) { dnsaddrs[i] = NULL; }
    count = 0;

    if (resolv_conf_dnsaddrs != NULL) {
	    for (i = 0; resolv_conf_dnsaddrs[i] != NULL; count++, i++) {
		    dnsaddrs[count] = strdup(resolv_conf_dnsaddrs[i]);
	    }
    }

    for (i = 0; (interface = __lsck_interface[i]) != NULL; i++) {
	    if (interface->addr == NULL)
		    continue;
	    if (interface->addr->family != AF_INET)
		    continue;

	    for (j = 0; (inet = interface->addr->table.inet[j]) != NULL; j++) {
		    for (k = 0; k < LSCK_IF_ADDR_INET_DNS_MAX; k++) {
			    if (inet->dns[k].s_addr == 0)
				    break;
			    dnsaddrs[count] = strdup(inet_ntoa(inet->dns[k]));
			    count++;
		    }
	    }
    }

    /* Free the resolv.conf structures */
    if (resolv_conf_dnsaddrs != NULL) {
	    for (i = 0; resolv_conf_dnsaddrs[i] != NULL; i++) {
		    free(resolv_conf_dnsaddrs[i]);
	    }
    }
    free(resolv_conf_dnsaddrs);

    /* Done */
    return (dnsaddrs);
}
