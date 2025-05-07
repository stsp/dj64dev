/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


#include "i_priv.h"     /* RD: inetprivate.h -> i_priv.h */
#include <netdb.h>
#include <sys/uio.h>	/* RD: writev() decl */

static const char *h_errlist[] = {
	"Error 0",
	"Unknown host",				/* 1 HOST_NOT_FOUND */
	"Host name lookup failure",		/* 2 TRY_AGAIN */
	"Unknown server error",			/* 3 NO_RECOVERY */
	"No address associated with name",	/* 4 NO_ADDRESS */
};

static int	h_nerr = { sizeof(h_errlist)/sizeof(h_errlist[0]) };

/*
 * herror --
 *	print the error indicated by the h_errno value.
 */
void
herror(char *s)
{
	struct iovec iov[4];
	struct iovec *v = iov;
	char delim[] = ": ";
	char nl[] = "\n";
	char herr[1024];
#if NLS
        libc_nls_init();
#endif

	if (s && *s) {
		v->iov_base = s;
		v->iov_len = strlen(s);
		v++;
		v->iov_base = delim;
		v->iov_len = 2;
		v++;
	}
	strlcpy(herr, h_errno < h_nerr ?
	    h_errlist[h_errno] : "Unknown error", sizeof(herr));
	v->iov_base = herr;
	v->iov_len = strlen(v->iov_base);
	v++;
	v->iov_base = nl;
	v->iov_len = 1;
#if NLS
	if ((u_int)h_errno < h_nerr)
		fprintf(stderr, (s && *s) ? "%s: %s\n":"%s\n",
			(s && *s) ? s: catgets(_libc_cat, HerrorListSet, h_errno+1, (char *) h_errlist[h_errno]),
			(s && *s) ? catgets(_libc_cat, HerrorListSet, h_errno+1, (char *) h_errlist[h_errno]): NULL);
	else
		fprintf(stderr, (s && *s) ? "%s: %s\n":"%s\n",
			(s && *s) ? s: catgets(_libc_cat, ErrorListSet, 1, "Unknown error"),
			(s && *s) ? catgets(_libc_cat, ErrorListSet, 1, "Unknown error"): NULL);

#else
	writev(2, iov, (v - iov) + 1);
#endif

}
