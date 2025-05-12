/*
 * Copyright (c) 1985, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* RD: Modifications by Richard Dawe for the libsocket project. Modifications
   marked with 'RD' comments like this one. */

#include "i_priv.h"         /* RD: inetprivate.h -> i_priv.h */

#include <resolv.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <limits.h>         /* RD: For PATH_MAX                            */
#include <lsck/lsck.h>      /* RD: Use configuration routines              */
#include <lsck/hostname.h>  /* RD: Use modified registry-based gethostname */
#include <lsck/ini.h>       /* RD: .ini and .cfg routines                  */

#if defined(USE_OPTIONS_H)
# include "r_opts.h"    /* RD: res_options.h -> r_opts.h */
#endif

#define	MAXALIASES	35
#define	MAXADDRS	35
#define MAXTRIMDOMAINS  4

/* IM: #define _PATH_HOSTCONF	__PATH_ETC_INET"/host.conf" */

/* Strict but flexible checking in _PATH_HOSTCONF file.

 * (+) Keywords need not start at the begining of the line.
 * (+) Invalid keywords are warned.
 * (+) Invalid booleans (other than "on" or "off") are warned.

 * Mitch DSouza <m.dsouza@mrc-apu.cam.ac.uk> - 30th October 1993
 */
#define STRICT

#define SERVICE_NONE  0
#define SERVICE_BIND  1
#define SERVICE_HOSTS 2
#define SERVICE_NIS   3
#define SERVICE_MAX   3

#define CMD_ORDER       "order"
#define CMD_TRIMDOMAIN  "trim"
#define CMD_HMA         "multi"
#define CMD_SPOOF       "nospoof"
#define CMD_SPOOFALERT  "alert"
#define CMD_REORDER     "reorder"
#define CMD_ON          "on"
#define CMD_OFF         "off"
#define CMD_WARN        "warn"
#define CMD_NOWARN      "warn off"

#define ORD_BIND        "bind"
#define ORD_HOSTS       "hosts"
#define ORD_NIS         "nis"

#define ENV_HOSTCONF    "RESOLV_HOST_CONF"
#define ENV_SERVORDER   "RESOLV_SERV_ORDER"
#define ENV_SPOOF       "RESOLV_SPOOF_CHECK"
#define ENV_TRIM_OVERR  "RESOLV_OVERRIDE_TRIM_DOMAINS"
#define ENV_TRIM_ADD    "RESOLV_ADD_TRIM_DOMAINS"
#define ENV_HMA         "RESOLV_MULTI"
#define ENV_REORDER     "RESOLV_REORDER"

#define TOKEN_SEPARATORS " ,;:"

static int service_order[SERVICE_MAX + 1];
static int service_done = 0;

//static const char AskedForGot[] =
//			  "gethostby*.getanswer: asked for \"%s\", got \"%s\"";

static char *h_addr_ptrs[MAXADDRS + 1];

static struct hostent host;
static char *host_aliases[MAXALIASES];
static char hostbuf[BUFSIZ+1];
static struct in_addr host_addr;
static char HOSTDB[PATH_MAX];               /* RD: Don't need 1K for this! */
static FILE *hostf = NULL;
static char hostaddr[MAXADDRS];
static char *host_addrs[2];
static int stayopen = 0;
static int hosts_multiple_addrs = 0;
static int spoof = 0;
static int spoofalert = 0;
static int reorder = 0;
static char *trimdomain[MAXTRIMDOMAINS];
static char trimdomainbuf[BUFSIZ];
static int numtrimdomains = 0;
#ifdef NIS
static struct hostent *_getnishost __P((char *, char*));
#endif

#ifdef RESOLVSORT
static void addrsort __P((char **, int));
#endif

#if PACKETSZ > 1024
#define	MAXPACKET	PACKETSZ
#else
#define	MAXPACKET	1024
#endif

typedef union {
    HEADER hdr;
    u_char buf[MAXPACKET];
} querybuf;

typedef union {
    long al;
    char ac;
} align;

#if NLS
#include "nl_types.h"
#endif

extern char *__libc_secure_getenv(const char *);

extern int yp_match   (char *, char *, char *, int, char **, int *);
extern int yp_first   (char *, char *, char **, int *, char **, int *);
extern int yp_next    (char *, char *, char *, int, char **,
                       int *, char **, int *);
extern int yp_get_default_domain   (char **);

struct hostent *_gethtbyaddr(const char *, int, int );
struct hostent *_gethtbyname( const char * );
struct hostent *_gethtent( void );
void _endhtent( void );
void _sethtent( int );
struct hostent *gethostbyaddr(const char *, int, int );
struct hostent *gethostbyname(const char *);
struct hostent *gethostent( void );
static struct hostent *getanswer( const querybuf *, int,
											const char *, int, int );
static void init_services( void );
static void reorder_addrs( struct hostent * );
static struct hostent *trim_domains( struct hostent * );
static void dotrimdomain( char * );

static void
dotrimdomain( char *c )
{
#if 0
	/* assume c points to the start of a host name; trim off any
	   domain name matching any of the trimdomains */
	int d,l1,l2;

	for(d=0;d<numtrimdomains;d++){
		l1=strlen(trimdomain[d]);
		l2=strlen(c);
		if(l2>l1 && !strcasecmp(c+l2-l1,trimdomain[d]))
			*(c+(strlen(c)-l1))='\0';
	}
#endif
}

static struct hostent *
trim_domains( struct hostent *h )
{
#if 0
	if(numtrimdomains){
		int i;
        dotrimdomain((char *) h->h_name);
		for(i=0;h->h_aliases[i];i++)
			dotrimdomain(h->h_aliases[i]);
	}
#endif
	return(h);
}


#if NLS
#define bad_config_format(cmd,line) \
    fprintf(stderr, catgets(_libc_cat, NetMiscSet, \
		NetMiscResolvIncorrectFormat, \
		"resolv+: %s:%d: command incorrectly formatted.\n"), \
		hostconf, line);	/* for now, don't print cmd */
#else
#define bad_config_format(cmd,line) \
    fprintf(stderr, "resolv+: %s:%d: command incorrectly formatted.\n", hostconf, line);
#endif

/* static void debug_breakout(x)
long x;
{
	register int a, b, c, d;

	d = x & 255; x >>= 8;
	c = x & 255; x >>= 8;
	b = x & 255; x >>= 8;
	a = x & 255; x >>= 8;
	fprintf(stderr, "resolv+: %d.%d.%d.%d", a,b,c,d );

	return;
} */

/* reorder_addrs -- by Tom Limoncelli
	Optimize order of an address list.

	gethostbyaddr() usually returns a list of addresses in some
	arbitrary order.  Most programs use the first one and throw the
	rest away.  This routine attempts to find a "best" address and
	swap it into the first position in the list.  "Best" is defined
	as "an address that is on a local subnet".  The search ends after
	one "best" address is found.  If no "best" address is found,
	nothing is changed.

	On first execution, a table is built of interfaces, netmasks,
	and mask'ed addresses.  This is to speed up future queries but
	may require you to reboot after changing internet addresses.
	(doesn't everyone reboot after changing internet addresses?)

	This routine should not be called if gethostbyaddr() is about
	to return only one address.

*/

/* Hal Stern (June 1992) of Sun claimed that more than 4 ethernets in a
Sun 4/690 would not work.  This variable is set to 10 to accomodate our
version of reality */
/* Alan Cox recommends now (Aug 1996) at minimum 64 since it's not unusual to
have about 250 of them... */
#define MAXINTERFACES (64)

static void
reorder_addrs( struct hostent *h )
{
 return; /* IM: the simplest way */
}

static void
init_services( void )
{
	char *cp, *dp, buf[BUFSIZ];
	register int cc = 0;
	FILE *fd;
	char *tdp = trimdomainbuf;
	char *hostconf;
    int lineno, errs;
	FILE *myfd; /* IM: */
    char *x;
    char hostconf_build[1024];  /* RD: Use a better name */

    /* RD: Use configuration routines now */
    *hostconf_build = '\0';
    x = __lsck_config_getfile();
    if (x != NULL) {
        GetPrivateProfileString("main", "host.conf", NULL,
                                hostconf_build, sizeof(hostconf_build), x);

        /* Diagnostic error message */
	if (__lsck_debug_enabled()) {
            if (*hostconf_build == '\0')
                fprintf(stderr,
                        "libsocket: Unable to find host.conf in %s!\n", x);
            else
                printf("libsocket: Using '%s' as host.conf\n", hostconf_build);
	}
    }

#if NLS
	libc_nls_init();
#endif

    /* RD: Environmental override */
	if(NULL==(hostconf=__libc_secure_getenv(ENV_HOSTCONF))){
	  hostconf = hostconf_build;
	}

    myfd = fopen(hostconf, "rt");
	if (myfd != NULL) {
		struct stat sb;
		/* Sorry, only world readable files.
		 * I wouldn't like it to be tricked by smart
		 * "secure" setuid hackery. */
		if (fstat(fileno(myfd), &sb) || !(sb.st_mode & S_IROTH)) {
			fclose(myfd);
			myfd = NULL;
		}
	}
	if (myfd == NULL) {
        /* RD: I think assuming we have a DNS is a bad idea */
		/* RD: Do we have at least one DNS server? If so, include
		 * DNS in the default services list. */
		x = __lsck_getdnsaddr();

		if (x == NULL) {
			service_order[0] = SERVICE_HOSTS;
			service_order[1] = SERVICE_NONE;
		} else {
			free(x);
			service_order[0] = SERVICE_HOSTS;
			service_order[1] = SERVICE_BIND;
			service_order[2] = SERVICE_NONE;
		}
	} else {
		fd = myfd;
		errs = lineno = 0;
		while (fgets(buf, BUFSIZ, fd) != NULL) {
			if (errs) {
				/* FIXME: NLS */
				fprintf(stderr, "resolv+: errors found, parsing aborted\n");
				break;
			}
			lineno++;
			if ((cp = rindex(buf, '\n')) != NULL)
				*cp = '\0';
			if (buf[0] == '#')
				continue;

#ifdef STRICT
#define checkbuf(b, cmd) (!strncasecmp(b, cmd, strlen(cmd)) && \
			  (!b[strlen(cmd)] || isspace(b[strlen(cmd)])))
#else
#define checkbuf(b, cmd) (!strncasecmp(b, cmd, strlen(cmd)))
#endif


			for (cp=buf; cp && *cp && isspace(*cp); cp++);
			if  (cp && *cp) {
				memmove(buf, cp, strlen(cp) + 1);
				if (checkbuf(buf, CMD_ORDER)) {
					cp = strpbrk(buf, " \t");
					if (!cp
#ifdef STRICT
						 || !*(cp+1)
#endif
						 ) {
						errs++; bad_config_format(CMD_ORDER, lineno);
					} else {
						do {
							while (*cp == ' ' || *cp == '\t')
									cp++;
							dp = strpbrk(cp, TOKEN_SEPARATORS);
							if (dp) *dp = '\0';
							if (checkbuf(cp, ORD_BIND)) {
								service_order[cc++] = SERVICE_BIND;
								if (!(_res.options & RES_INIT))
										res_init();
							} else if (checkbuf(cp, ORD_HOSTS))
									service_order[cc++] = SERVICE_HOSTS;
							else if (checkbuf(cp, ORD_NIS))
									service_order[cc++] = SERVICE_NIS;
							else {
								errs++; bad_config_format(CMD_ORDER, lineno);
#if NLS
								fprintf(stderr, catgets(_libc_cat, NetMiscSet,
																NetMiscResolvInvalid,
						"resolv+: %s:%d: invalid keyword\n"), hostconf, lineno);
								fprintf(stderr, catgets(_libc_cat, NetMiscSet,
																NetMiscResolvValid,
						"resolv+: valid keywords are: %s, %s and %s\n"),
										  ORD_BIND, ORD_HOSTS, ORD_NIS);
#else
								fprintf(stderr,
						"resolv+: %s:%d is an invalid keyword\n", hostconf, lineno);
								fprintf(stderr,
										  "resolv+: valid keywords are: %s, %s and %s\n",
										  ORD_BIND, ORD_HOSTS, ORD_NIS);
#endif

							}

							if (dp) cp = ++dp;
						} while (dp != NULL);
						if (cc == 0) {
							errs++; bad_config_format(CMD_ORDER, lineno);
#if NLS
							fprintf(stderr,catgets(_libc_cat, NetMiscSet,
														  NetMiscUnrecognized,
"resolv+: search order not specified or unrecognized keyword, host resolution will fail.\n"));
#else
							fprintf(stderr,
"resolv+: search order not specified or unrecognized keyword, host resolution will fail.\n");
#endif
						}
					}

				} else if (checkbuf(buf, CMD_HMA)) {
#ifdef STRICT

#if NLS
#define check_legal(key,val) { \
				for (cp=strpbrk(buf, " \t"); \
				    cp && *cp && isspace(*cp); cp++); \
				if (cp && *cp) {\
				if (strlen(CMD_ON) == strlen(cp) && \
				    checkbuf(cp,CMD_ON)) val = 1; \
				else if (strlen(CMD_OFF) == strlen(cp) && \
					 checkbuf(cp,CMD_OFF)) val = 0; \
				else { \
					errs++; bad_config_format(key, lineno); \
					fprintf(stderr, catgets(_libc_cat, NetMiscSet, \
						NetMiscResolvInvalid, \
						"resolv+: %s:%d: invalid keyword\n"), hostconf, lineno); \
				     } \
				} else { errs++; bad_config_format(key, lineno); } \
			}
#else

#define check_legal(key,val) { \
				for (cp=strpbrk(buf, " \t"); \
				    cp && *cp && isspace(*cp); cp++); \
				if (cp && *cp) { \
				if (strlen(CMD_ON) == strlen(cp) && \
				    checkbuf(cp,CMD_ON)) val = 1; \
				else if (strlen(CMD_OFF) == strlen(cp) && \
					 checkbuf(cp,CMD_OFF)) val = 0; \
				else  { \
					errs++; bad_config_format(key, lineno); \
               fprintf(stderr, "resolv+: %s:%d is an invalid keyword\n", hostconf, lineno); \
				} \
			} else { errs++; bad_config_format(key, lineno); } \
		}
#endif /* NLS */
					check_legal(CMD_HMA,hosts_multiple_addrs);

#else /* !STRICT */
					if ( cp = strpbrk(buf, " \t") ) {
						while (*cp == ' ' || *cp == '\t') cp++;
						if (checkbuf(cp, CMD_ON))
								hosts_multiple_addrs = 1;
					} else {
						errs++; bad_config_format(CMD_HMA, lineno);
					}
#endif /* STRICT */

				} else if (checkbuf(buf, CMD_SPOOF)) {
#ifdef STRICT
					check_legal(CMD_SPOOF,spoof);
#else
					if ( cp = strpbrk(buf, " \t") ) {
						while (*cp == ' ' || *cp == '\t') cp++;
						if (checkbuf(cp, CMD_ON))
								spoof = 1;
					} else {
						errs++; bad_config_format(CMD_SPOOF, lineno);
					}
#endif


				} else if (checkbuf(buf, CMD_SPOOFALERT)) {
#ifdef STRICT
					check_legal(CMD_SPOOFALERT,spoofalert);
#else
					if ( cp = strpbrk(buf, " \t") ) {
						while (*cp == ' ' || *cp == '\t') cp++;
						if (checkbuf(cp, CMD_ON))
								spoofalert = 1;
					} else {
						errs++; bad_config_format(CMD_SPOOFALERT, lineno);
					}
#endif

				} else if (checkbuf(buf, CMD_REORDER)) {
#ifdef STRICT
					check_legal(CMD_REORDER,reorder);
#else
					if (cp = strpbrk(buf, " \t")) {
						while (*cp == ' ' || *cp == '\t') cp++;
						if (checkbuf(cp, CMD_ON))
								reorder = 1;
					} else {
						errs++; bad_config_format(CMD_REORDER, lineno);
					}
#endif

				} else if (checkbuf(buf, CMD_TRIMDOMAIN)) {
					if(numtrimdomains<MAXTRIMDOMAINS){
						if (( cp = strpbrk(buf, " \t" ))) {
							while (*cp == ' ' || *cp == '\t') cp++;
							if (*cp) {
								/* sec fix */
								if (strlen(cp)+tdp-trimdomainbuf
									 < (int)sizeof(trimdomainbuf)-2) {
									(void) strcpy(tdp,cp);
									trimdomain[numtrimdomains++]=tdp;
									tdp += strlen(cp)+1;
								}
							} else {
								errs++; bad_config_format(CMD_TRIMDOMAIN, lineno);
							}
						} else {
							errs++; bad_config_format(CMD_TRIMDOMAIN, lineno);
						}
					}
				}
#ifdef STRICT
				else {
					if ((cp = strpbrk(buf, " \t"))) *cp='\0';
					errs++;
#if NLS
					fprintf(stderr, catgets(_libc_cat, NetMiscSet,
													NetMiscResolvInvalid,
						"resolv+: %s:%d: invalid keyword\n"), hostconf, lineno);
#else
					fprintf(stderr, "resolv+: %s:%d: invalid keyword\n", buf, lineno );
#endif
				}
#endif
			}
		}
		service_order[cc] = SERVICE_NONE;
		fclose(fd);
	}
	/* override service_order if environment variable */
	if(NULL!=(cp=__libc_secure_getenv(ENV_SERVORDER))) {
		cc=0;
		if(NULL!=(cp=strtok(cp, TOKEN_SEPARATORS))){
			do{
				if(checkbuf(cp, ORD_BIND)) {
					service_order[cc++] = SERVICE_BIND;
					if (!(_res.options & RES_INIT))
						res_init();
				} else if (checkbuf(cp, ORD_HOSTS))
					service_order[cc++] = SERVICE_HOSTS;
				else if (checkbuf(cp, ORD_NIS))
					service_order[cc++] = SERVICE_NIS;
			} while((cp=strtok(NULL, TOKEN_SEPARATORS)));
		service_order[cc] = SERVICE_NONE;
		}
	}
	/* override spoof if environment variable */
	if(NULL!=(cp=__libc_secure_getenv(ENV_SPOOF))) {
		if(checkbuf(cp, CMD_WARN)){
			spoof=1;
			spoofalert=1;
		} else if (checkbuf(cp, CMD_OFF)){
			spoof=0;
			spoofalert=0;
		} else if (checkbuf(cp, CMD_NOWARN)){
			spoof=1;
			spoofalert=0;
		} else {
			spoof=1;
		}
	}

    /* RD: Tidied up slightly - Added braces round if statements to avoid
       ambiguity */

	/* override hma if environment variable */
    if ((cp = __libc_secure_getenv(ENV_HMA)) != NULL) {
        if (checkbuf(cp, CMD_ON))
			hosts_multiple_addrs=1;
		else
			hosts_multiple_addrs=0;
    }

	/* override reorder if environment variable */
    if ((cp = __libc_secure_getenv(ENV_REORDER)) != NULL) {
		if (checkbuf(cp, CMD_ON))
			reorder = 1;
		else
			reorder = 0;
    }

    /* End RD */

	/* add trimdomains from environment variable */
	if(NULL!=(cp=__libc_secure_getenv(ENV_TRIM_ADD))) {
		if(NULL!=(cp=strtok(cp, TOKEN_SEPARATORS))){
			do{
				if(numtrimdomains<MAXTRIMDOMAINS){
					/* sec fix */
					if (strlen(cp)+tdp-trimdomainbuf < (int)sizeof(trimdomainbuf)-2) {
						(void)strcpy(tdp, cp);
						trimdomain[numtrimdomains++]=tdp;
						tdp += strlen(cp)+1;
					}
				}
			} while((cp=strtok(NULL, TOKEN_SEPARATORS)));
		}
	}

	/* override trimdomains from environment variable */
	if(NULL!=(cp=__libc_secure_getenv(ENV_TRIM_OVERR))) {
		numtrimdomains=0;
		tdp=trimdomainbuf;
		if(NULL!=(cp=strtok(cp, TOKEN_SEPARATORS))){
			do{
				if(numtrimdomains<MAXTRIMDOMAINS){
					/* sec fix */
					if (strlen(cp)+tdp-trimdomainbuf < (int)sizeof(trimdomainbuf)-2) {
						(void)strcpy(tdp, cp);
						trimdomain[numtrimdomains++]=tdp;
						tdp += strlen(cp)+1;
					}
				}
			} while((cp=strtok(NULL, TOKEN_SEPARATORS)));
		}
	}

	service_done = 1;
}

static struct hostent *
getanswer( const querybuf *answer, int anslen, const char *qname,
			 int qclass, int qtype )
{
	register const HEADER *hp;
	register const u_char *cp;
	register int n;
	const u_char *eom;
	char *bp, **ap, **hap;
	int type, class, buflen, ancount, qdcount;
	int haveanswer, had_error;
	int toobig = 0;
	char tbuf[MAXDNAME+1];
//	const char *tname;

//	tname = qname;
	host.h_name = NULL;
	eom = answer->buf + anslen;
	/*
	 * find first satisfactory answer
	 */
	hp = &answer->hdr;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	bp = hostbuf;
	buflen = sizeof hostbuf;
	cp = answer->buf + HFIXEDSZ;
	if (qdcount != 1) {
		h_errno = NO_RECOVERY;
		return (NULL);
	}
	if ((n = dn_expand(answer->buf, eom, cp, bp, buflen)) < 0) {
		h_errno = NO_RECOVERY;
		return (NULL);
	}
#if NLS
	libc_nls_init();
#endif
	cp += n + QFIXEDSZ;
	if (qtype == T_A) {
		/* res_send() has already verified that the query name is the
		 * same as the one we sent; this just gets the expanded name
		 * (i.e., with the succeeding search-domain tacked on).
		 */
		n = strlen(bp) + 1;		/* for the \0 */
		host.h_name = bp;
		bp += n;
		buflen -= n;
		/* The qname can be abbreviated, but h_name is now absolute. */
		qname = host.h_name;
	}
	ap = host_aliases;
	*ap = NULL;
	host.h_aliases = host_aliases;
	hap = h_addr_ptrs;
	*hap = NULL;
#if BSD >= 43 || defined(h_addr)	/* new-style hostent structure */
	host.h_addr_list = h_addr_ptrs;
#endif
	haveanswer = 0;
	had_error = 0;
	while (ancount-- > 0 && cp < eom && !had_error) {
		n = dn_expand(answer->buf, eom, cp, bp, buflen);
		if (n < 0) {
			had_error++;
			continue;
		}
		cp += n;			/* name */
		type = _getshort(cp);
 		cp += INT16SZ;			/* type */
		class = _getshort(cp);
 		cp += INT16SZ + INT32SZ;	/* class, TTL */
		n = _getshort(cp);
		cp += INT16SZ;			/* len */
		if (class != qclass) {
			/* XXX - debug? syslog? */
			cp += n;
			continue;		/* XXX - had_error++ ? */
		}
		if (qtype == T_A && type == T_CNAME) {
			if (ap >= &host_aliases[MAXALIASES-1])
				continue;
			n = dn_expand(answer->buf, eom, cp, tbuf, sizeof tbuf);
			if (n < 0) {
				had_error++;
				continue;
			}
			cp += n;
/*			if (host.h_name && strcasecmp(host.h_name, bp) != 0) {
				syslog(LOG_NOTICE|LOG_AUTH,
#if NLS
		catgets(_libc_cat, NetMiscSet, NetMiscAskedForGotCNAME,
		"gethostby*.getanswer: asked for \"%s\", got CNAME for \"%s\""),

#else
		"gethostby*.getanswer: asked for \"%s\", got CNAME for \"%s\"",
#endif
				       host.h_name, bp);
				continue;
			} */
			/* Store alias. */
			*ap++ = bp;
			n = strlen(bp) + 1;	/* for the \0 */
			bp += n;
			buflen -= n;
			/* Get canonical name. */
			n = strlen(tbuf) + 1;	/* for the \0 */
			if (n > buflen) {
				had_error++;
				continue;
			}
			strcpy(bp, tbuf);	/* cannot overflow */
			host.h_name = bp;
			bp += n;
			buflen -= n;
			continue;
		}
		if (qtype == T_PTR && type == T_CNAME) {
			n = dn_expand(answer->buf, eom, cp, tbuf, sizeof tbuf);
                        if (n < 0) {
				had_error++;
                                continue;
                        }
                        cp += n;
                        /* Get canonical name. */
                        n = strlen(tbuf) + 1;   /* for the \0 */
                        if (n > buflen) {
				had_error++;
                                continue;
                        }
                        strcpy(bp, tbuf);	/* cannot overflow */
//                        tname = bp;
                        bp += n;
                        buflen -= n;
                        continue;
                }
/* IM: no need		if (type != qtype) {
			syslog(LOG_NOTICE|LOG_AUTH,
#if NLS
		catgets(_libc_cat, NetMiscSet, NetMiscAskedForType,
		     "gethostby*.getanswer: asked for type %d(%s), got %d(%s)"),
#else
		     "gethostby*.getanswer: asked for type %d(%s), got %d(%s)",
#endif
			       qtype, qname, type, bp);
			cp += n;
			continue;
		} */
		switch (type) {
		case T_PTR:
/*			if (strcasecmp(tname, bp) != 0) {
				syslog(LOG_NOTICE|LOG_AUTH,
#if NLS
	       catgets(_libc_cat, NetMiscSet, NetMiscAskedForGot, AskedForGot),
#else
				       AskedForGot,
#endif
				       qname, bp);
				cp += n;
				continue;
			} */
			n = dn_expand(answer->buf, eom, cp, bp, buflen);
			if (n < 0) {
				had_error++;
				break;
			}
#if MULTI_PTRS_ARE_ALIASES
			cp += n;
			if (!haveanswer)
				host.h_name = bp;
			else if (ap < &host_aliases[MAXALIASES-1])
				*ap++ = bp;
			else
				n = -1;
			if (n != -1) {
				n = strlen(bp) + 1;	/* for the \0 */
				bp += n;
				buflen -= n;
			}
			break;
#else
			host.h_name = bp;
			return (&host);
#endif
		case T_A:
/*			if (strcasecmp(host.h_name, bp) != 0) {
				syslog(LOG_NOTICE|LOG_AUTH,
#if NLS
	       catgets(_libc_cat, NetMiscSet, NetMiscAskedForGot, AskedForGot),
#else
				       AskedForGot,
#endif
				       host.h_name, bp);
				cp += n;
				continue;
			} */
			if (haveanswer) {
				if (n != host.h_length) {
					cp += n;
					continue;
				}
			} else {
				register int nn;

				host.h_length = n;
				host.h_addrtype = (class == C_IN)
						  ? AF_INET
						  : AF_UNSPEC;
				host.h_name = bp;
				nn = strlen(bp) + 1;	/* for the \0 */
				bp += nn;
				buflen -= nn;
			}

			bp += sizeof(align) - ((u_long)bp % sizeof(align));

			if (bp + n >= &hostbuf[sizeof hostbuf]) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					printf("size (%d) too big\n", n);
#endif
				had_error++;
				continue;
			}
			if (hap >= &h_addr_ptrs[MAXADDRS-1]) {
				if (_res.options & RES_DEBUG && !toobig++)
					printf("Too many addresses (%d)\n",
					       MAXADDRS);
				cp += n;
				continue;
			}
			bcopy(cp, *hap++ = bp, n);
			bp += n;
			cp += n;
			break;
		default:
			abort();
		} /*switch*/
		if (!had_error)
			haveanswer++;
	} /*while*/
	if (haveanswer) {
		*ap = NULL;
		*hap = NULL;
# if defined(RESOLVSORT)
		/*
		 * Note: we sort even if host can take only one address
		 * in its return structures - should give it the "best"
		 * address in that case, not some random one
		 */
		if (_res.nsort && haveanswer > 1 &&
		    qclass == C_IN && qtype == T_A)
			addrsort(h_addr_ptrs, haveanswer);
# endif /*RESOLVSORT*/
#if BSD >= 43 || defined(h_addr)	/* new-style hostent structure */
		/* nothing */
#else
		host.h_addr = h_addr_ptrs[0];
#endif /*BSD*/
		if (!host.h_name) {
			n = strlen(qname) + 1;	/* for the \0 */
			/* I'm not sure about this one. */
			strcpy(bp, qname);
			host.h_name = bp;
		}
		return (&host);
	} else {
		h_errno = TRY_AGAIN;
		return (NULL);
	}
}

struct hostent *
gethostent( void )
{
  int cc;
  struct hostent *hp;

  if (!service_done)
    init_services();

  for (cc = 0; service_order[cc] != SERVICE_NONE &&
	cc <= SERVICE_MAX; cc++)
  {
    switch (service_order[cc])
    {
      case SERVICE_BIND:
	break;

      case SERVICE_HOSTS:
	hp = _gethtent ();
	if (h_addr_ptrs[1] && reorder)
	  reorder_addrs(hp);
	if (hp)
	{
	  h_errno = NETDB_SUCCESS;
	  return hp;
	}
	h_errno = HOST_NOT_FOUND;
	break;

#ifdef NIS
      case SERVICE_NIS:
	hp = _getnishost(NULL, "hosts.byname");
	if (h_addr_ptrs[1] && reorder)
	  reorder_addrs(hp);
	if (hp)
	{
	  h_errno = NETDB_SUCCESS;
	  return hp;
	}
	h_errno = HOST_NOT_FOUND;
	break;
#endif /* NIS */

    }
  }
  return ((struct hostent *) NULL);
}

struct hostent *
gethostbyname(const char *name)
{
	querybuf buf;
	const char *cp;
	register int cc;
	int n;
	struct hostent *hp;

	/*
	 * disallow names consisting only of digits/dots, unless
	 * they end in a dot.
	 */
	if (isdigit(name[0]))
		for (cp = name;; ++cp) {
			if (!*cp) {
				if (*--cp == '.')
					break;
				/*
				 * All-numeric, no dot at the end.
				 * Fake up a hostent as if we'd actually
				 * done a lookup.  What if someone types
				 * 255.255.255.255?  The test below will
				 * succeed spuriously... ???
				 */
				if ((host_addr.s_addr = inet_addr(name)) == INADDR_NONE) {
					h_errno = HOST_NOT_FOUND;
					return((struct hostent *) NULL);
				}
				host.h_name = name;
				host.h_aliases = host_aliases;
				host_aliases[0] = NULL;
				host.h_addrtype = AF_INET;
				host.h_length = sizeof(u_long);
				h_addr_ptrs[0] = (char *)&host_addr;
				h_addr_ptrs[1] = (char *)0;
#if BSD >= 43 || defined(h_addr)	/* new-style hostent structure */
				host.h_addr_list = h_addr_ptrs;
#else
				host.h_addr = h_addr_ptrs[0];
#endif
				h_errno = NETDB_SUCCESS;
				return (&host);
			}
			if (!isdigit(*cp) && *cp != '.')
				break;
		}

	if (!service_done)
		init_services();

	for (cc = 0; service_order[cc] != SERVICE_NONE &&
	     cc <= SERVICE_MAX; cc++) {
		switch (service_order[cc]) {
		case SERVICE_BIND:
			if ((n = res_search(name, C_IN, T_A,
					    buf.buf, sizeof(buf.buf))) < 0) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					printf("res_search failed\n");
#endif
				break;
			}
			hp = getanswer(&buf, n, name, C_IN, T_A);
			if (h_addr_ptrs[1] && reorder)
				reorder_addrs(hp);
			if (hp)
			{
				h_errno = NETDB_SUCCESS;
				return trim_domains(hp);
			}
			break;
		case SERVICE_HOSTS:
			if (*trimdomain) {
				char *p;
				p = strdup(name);
				dotrimdomain(p);
				hp = _gethtbyname(p);
				free(p);
			}
			else
			{
				hp = _gethtbyname(name);
			}
			if (h_addr_ptrs[1] && reorder)
				reorder_addrs(hp);
			if (hp)
			{
				h_errno = NETDB_SUCCESS;
				return trim_domains(hp);
			}
			h_errno = HOST_NOT_FOUND;
			break;
#ifdef NIS
		case SERVICE_NIS:
			if (*trimdomain) {
				char *p;
				p = strdup(name);
				dotrimdomain(p);
				hp = _getnishost(p, "hosts.byname");
				free(p);
			} else
				hp = _getnishost(name, "hosts.byname");
			if (h_addr_ptrs[1] && reorder)
				reorder_addrs(hp);
			if (hp)
			{
				h_errno = NETDB_SUCCESS;
				return trim_domains(hp);
			}
			h_errno = HOST_NOT_FOUND;
			break;
#endif /* NIS */
		}
	}
	return ((struct hostent *) NULL);
}

struct hostent *
gethostbyaddr(const char *addr, int len, int type)
{
	int n;
	querybuf buf;
	register int cc;
	register struct hostent *hp;
	char qbuf[MAXDNAME];

	if (type != AF_INET) {
		h_errno = NETDB_INTERNAL;
		return ((struct hostent *) NULL);
	}

	if (!service_done)
	  init_services();

#if NLS
	libc_nls_init();
#endif
	cc = 0;
	while (service_order[cc] != SERVICE_NONE) {
	        switch (service_order[cc]) {
		case SERVICE_BIND:
			/* will not overflow */
			(void)sprintf(qbuf, "%u.%u.%u.%u.in-addr.arpa",
				      ((unsigned)addr[3] & 0xff),
				      ((unsigned)addr[2] & 0xff),
				      ((unsigned)addr[1] & 0xff),
				      ((unsigned)addr[0] & 0xff));
			n = res_query(qbuf, C_IN, T_PTR, (unsigned char *)&buf,
				      sizeof(buf));
			if (n < 0) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					printf("res_query failed\n");
#endif
				break;
			}
			hp = getanswer(&buf, n, qbuf, C_IN, T_PTR);
			if (hp) {
				if(spoof){
					/* Spoofing check code by
					 * Caspar Dik <casper@fwi.uva.nl>
					 */
					char nambuf[MAXDNAME+1];
					int ntd, namelen = strlen(hp->h_name);
					char **addrs;

					if (namelen >= MAXDNAME)
						return (struct hostent *)NULL;
					(void) strcpy(nambuf,hp->h_name);
					nambuf[namelen] = '.';
					nambuf[namelen+1] = '\0';

					/*
					 * turn off domain trimming,
					 * call gethostbyname(), then turn
					 * it back on, if applicable. This
					 * prevents domain trimming from
					 * making the name comparison fail.
					 */
					ntd=numtrimdomains;
					numtrimdomains=0;
					hp=gethostbyname(nambuf);
					numtrimdomains=ntd;
					nambuf[namelen] = 0;
					/*
					* the name must exist and the name
					* returned by gethostbyaddr must be
					* the canonical name and therefore
					* identical to the name returned by
					* gethostbyname()
					*/
					if (!hp || strcmp(nambuf, hp->h_name)){
						h_errno = HOST_NOT_FOUND;
						return (struct hostent *)NULL;
					}
					/*
					* now check the addresses
					*/
#if defined(h_addr) || BSD >= 43
					for (addrs = hp->h_addr_list;
						*addrs; addrs++){
						if (!bcmp(addrs[0], addr, len))
						{
							h_errno = NETDB_SUCCESS;
							return trim_domains(hp);
						}
					}
#else
					if (!bcmp(hp->h_addr, addr, len)))
					{
						h_errno = NETDB_SUCCESS;
						return trim_domains(hp);
					}
#endif
					/* We've been spoofed */
					h_errno = HOST_NOT_FOUND;
/*					if(spoofalert){
						openlog("resolv", LOG_PID,
						    LOG_AUTH);
						syslog(LOG_NOTICE,
#if NLS
						    catgets(_libc_cat, NetMiscSet,
							    NetMiscPossibleSpoof,
							    "gethostbyaddr: %s != %u.%u.%u.%u, possible spoof attempt"),
#else
						    "gethostbyaddr: %s != %u.%u.%u.%u, possible spoof attempt",
#endif
						    hp->h_name,
						    ((unsigned)addr[0]&0xff),
						    ((unsigned)addr[1]&0xff),
						    ((unsigned)addr[2]&0xff),
						    ((unsigned)addr[3]&0xff));
					} */
					return (struct hostent *)NULL;
				}
				hp->h_addrtype = type;
				hp->h_length = len;
				h_addr_ptrs[0] = (char *)&host_addr;
				h_addr_ptrs[1] = (char *)0;
				host_addr = *(const struct in_addr *)addr;
#if BSD < 43 && !defined(h_addr)	/* new-style hostent structure */
				hp->h_addr = h_addr_ptrs[0];
#endif
				h_errno = NETDB_SUCCESS;
				return trim_domains(hp);
			}
			h_errno = HOST_NOT_FOUND;
			break;
		case SERVICE_HOSTS:
			hp = _gethtbyaddr(addr, len, type);
			if (hp)
			{
				h_errno = NETDB_SUCCESS;
				return hp;
			}
			h_errno = HOST_NOT_FOUND;
			break;
#ifdef NIS
		case SERVICE_NIS:
			/* won't overflow */
			(void)sprintf(qbuf, "%u.%u.%u.%u",
				      ((unsigned)addr[0] & 0xff),
				      ((unsigned)addr[1] & 0xff),
				      ((unsigned)addr[2] & 0xff),
				      ((unsigned)addr[3] & 0xff));
			hp = _getnishost(qbuf, "hosts.byaddr");
			if (hp)
			{
				h_errno = NETDB_SUCCESS;
				return hp;
			}
			h_errno = HOST_NOT_FOUND;
			break;
#endif /* NIS */
		}
		cc++;
	}
	return ((struct hostent *)NULL);
}

void
_sethtent( int f )
{
	char *x;

    /* RD: Use configuration routines now */
    x = __lsck_config_getfile();
    if (x != NULL)
        GetPrivateProfileString("main", "hosts", NULL,
                                HOSTDB, sizeof(HOSTDB), x);
    else
        *HOSTDB = '\0';

    /*
       RD: If we can't find the specified file, look in the Windows
       directory.

       TODO: Add warning?
    */
    if (*HOSTDB == '\0') sprintf(HOSTDB, "%s/hosts", getenv("windir"));

    if (hostf == NULL) {
        hostf = fopen(HOSTDB, "rt" );

        /* RD: Warning message */
        if (hostf == NULL) perror("resolv: Unable to open hosts file");
    } else
		rewind(hostf);
	stayopen |= f;
}

void
_endhtent( void )
{
	if (hostf && !stayopen && !(_res.options & RES_STAYOPEN)) {
		(void) fclose(hostf);
		hostf = NULL;
	}
}

/* -------------
   - _gethtent -
   ------------- */

/* RD: Modified this so it doesn't use gotos & is easier to read. */

struct hostent *_gethtent (void)
{
	char *p;
	register char *cp, **q;

    if ( (hostf == NULL) && (hostf = fopen(HOSTDB, "rt" )) == NULL) {
 		h_errno = NETDB_INTERNAL;
		return (NULL);
	}

    /* Read the hosts file line-by-line*/
    while ((p = fgets(hostbuf, BUFSIZ, hostf)) != NULL) {
        /* Comment */
        if (*p == '#') continue;

        /* Look for comment/newline and nuke after that point */
        cp = strpbrk(p, "#\n");
        if (cp == NULL) continue;
        *cp = '\0';

        /* Look for whitespace at start and nuke it */
        cp = strpbrk(p, " \t");
        if (cp == NULL) continue;
        *cp++ = '\0';

        /* --- Match --- */

        /* THIS STUFF IS INTERNET SPECIFIC */
#if BSD >= 43 || defined(h_addr)    /* new-style hostent structure */
        host.h_addr_list = host_addrs;
#endif
        host.h_addr = hostaddr;
        *((u_long *)host.h_addr) = inet_addr(p);
        host.h_length = sizeof (u_long);
        host.h_addrtype = AF_INET;

        /* Find the host name */
        while (*cp == ' ' || *cp == '\t') cp++;
        host.h_name = cp;

        /* Find host aliases */
        q = host.h_aliases = host_aliases;
        cp = strpbrk(cp, " \t");
        if (cp != NULL) *cp++ = '\0';

        while (cp && *cp) {
            if (*cp == ' ' || *cp == '\t') {
                cp++;
                continue;
            }
            if (q < &host_aliases[MAXALIASES - 1]) *q++ = cp;
            cp = strpbrk(cp, " \t");
            if (cp != NULL) *cp++ = '\0';
        }

        *q = NULL;

        /* Done successfully */
        h_errno = NETDB_SUCCESS;
        return (&host);
    }

    /* EOF & no match */
    h_errno = HOST_NOT_FOUND;
    return (NULL);
}

/* End RD */

/* if hosts_multiple_addrs set, then gethtbyname behaves as follows:
 *  - for hosts with multiple addresses, return all addresses, such that
 *  the first address is most likely to be one on the same net as the
 *  host we're running on, if one exists.
 *  - like the dns version of gethostsbyname, the alias field is empty
 *  unless the name being looked up is an alias itself, at which point the
 *  alias field contains that name, and the name field contains the primary
 *  name of the host. Unlike dns, however, this behavior will still take place
 *  even if the alias applies only to one of the interfaces.
 *
 *	I changed to return aliase list no matter what. H.J.
 *
 *  - determining a "local" address to put first is dependant on the netmask
 *  being such that the least significant network bit is more significant
 *  than any host bit. Only strange netmasks will violate this.
 *  - we assume addresses fit into u_longs. That's quite internet specific.
 *  - if the host we're running on is not in the host file, the address
 *  shuffling will not take place.
 *                     - John DiMarco <jdd@cdf.toronto.edu>
 */
struct hostent *
_gethtbyname(const char *name)
/* _gethtbyname(const char *name) vch */
{
	register struct hostent *p;
	register char **cp;
	char **hap, **lhap, *bp, *lbp;
	int htbuflen, locbuflen;
	int found=0, localfound=0;
	char localname[MAXHOSTNAMELEN];

	static char htbuf[BUFSIZ+1]; /* buffer for host addresses */
	static char locbuf[BUFSIZ+1]; /* buffer for local hosts's addresses */
	static char *ht_addr_ptrs[MAXADDRS+1];
	static char *loc_addr_ptrs[MAXADDRS+1];
	static struct hostent ht;
	static char *aliases[MAXALIASES];
	static char namebuf[MAXHOSTNAMELEN];
	static char aliasebuf[BUFSIZ+1];
	int gap = aliasebuf - hostbuf;

	hap = ht_addr_ptrs;
	lhap = loc_addr_ptrs;
	*hap = NULL;
	*lhap = NULL;
	bp=htbuf;
	lbp=locbuf;
	htbuflen = sizeof(htbuf);
	locbuflen = sizeof(locbuf);

	aliases[0]=NULL;
	aliases[1]=NULL;
	(void) strncpy(namebuf, name, sizeof(namebuf));
	namebuf[sizeof(namebuf)-1] = 0;

    /* RD: Modified registry-based gethostname */
    (void) gethostname(localname, sizeof(localname));
    /* RD: End */

	_sethtent(0);
	while ((p = _gethtent())) {
		if (strcasecmp(p->h_name, name) == 0)
			found++;
		else
			for (cp = p->h_aliases; *cp != 0; cp++)
				if (strcasecmp(*cp, name) == 0){
					found++;
#if 0
					aliases[0]=name;
#endif
					/*
					 * This should not overflow, but it is
					 * better to be safe than sorry.
					 */
					(void) strncpy(namebuf, p->h_name,
						       sizeof(namebuf));
					namebuf[sizeof(namebuf)-1] = 0;
				}

		cp = p->h_aliases;
		if (found && aliases[0] == NULL && *cp)
		{
			unsigned int i;

			memcpy (aliasebuf, hostbuf, sizeof(aliasebuf));
			for (i = 0; *cp != 0 &&
				i < sizeof (aliases) / sizeof (aliases [0]) - 1;
				i++, cp++)
			{
				aliases [i] = *cp + gap;
			}
			aliases [i] = NULL;
		}

		if (strcasecmp(p->h_name, localname) == 0)
			localfound++;
		else
			for (cp=p->h_aliases; *cp != 0; cp++)
				if (strcasecmp(*cp, localname) == 0)
					localfound++;

		if(found){
			int n;

			if(!hosts_multiple_addrs){
				/* original behaviour requested */
				_endhtent();
				return(p);
			}
			n = p->h_length;

			ht.h_addrtype = p->h_addrtype;
			ht.h_length = p->h_length;

			if(n<=htbuflen){
				/* add the found address to the list */
				bcopy(p->h_addr_list[0], bp, n);
				*hap++=bp;
				*hap=NULL;
				bp+=n;
				htbuflen-=n;
			}
			found=0;
		}
		if(localfound){
			int n = p->h_length;
			if(n<=locbuflen){
				/* add the found local address to the list */
				bcopy(p->h_addr_list[0], lbp, n);
				*lhap++=lbp;
				*lhap=NULL;
				lbp+=n;
				locbuflen-=n;
			}
			localfound=0;
		}
	}
	_endhtent();

	if(NULL==ht_addr_ptrs[0]){
		return((struct hostent *)NULL);
	}

	ht.h_aliases = aliases;
	ht.h_name = namebuf;

	/* shuffle addresses around to ensure one on same net as local host
	   is first, if exists */
	{
		/* "best" address is assumed to be the one with the greatest
		   number of leftmost bits matching any of the addresses of
		   the local host. This assumes a netmask in which all net
		   bits precede host bits. Usually but not always a fair
		   assumption. */

		/* portability alert: assumption: iaddr fits in u_long.
		   This is really internet specific. */
		int i,j, best=0;
		u_long bestval = (u_long)~0;

		for(i=0;loc_addr_ptrs[i];i++){
			for(j=0;ht_addr_ptrs[j];j++){
				u_long t, l, h;
				/* assert(sizeof(u_long)>=ht.h_length); */
				bcopy(loc_addr_ptrs[i], (char *)&t,
					ht.h_length);
				l=ntohl(t);
				bcopy(ht_addr_ptrs[j], (char *)&h,
					ht.h_length);
				t=l^h;

				if(t<bestval){
					best=j;
					bestval=t;
				}
			}
		}
		if(best){
			char *tmp;

			/* swap first and best address */
			tmp=ht_addr_ptrs[0];
			ht_addr_ptrs[0]=ht_addr_ptrs[best];
			ht_addr_ptrs[best]=tmp;
		}
	}

	ht.h_addr_list = ht_addr_ptrs;
	return (&ht);
}

#ifdef NIS
static struct hostent *
_getnishost( char *name, char *map )
{
	register char *cp, **q;
	char *result;
	int resultlen;
	static int first = 1;
	static struct hostent h;
	static char *domain = (char *)NULL;
	static char *keyname;
	static int keylen;

	if (domain == (char *)NULL)
		if (yp_get_default_domain (&domain))
			return ((struct hostent *)NULL);

	if (name)
	{
	  if (yp_match(domain, map, name, strlen(name), &result, &resultlen))
		return ((struct hostent *)NULL);
	}
	else
	{
	  /* If it is the first. */
	  if (first)
	  {
	    if (yp_first(domain, map, &keyname, &keylen, &result,
		&resultlen))
	      return ((struct hostent *)NULL);
	    first = 0;
	  }
	  else
	  {
	    if (yp_next (domain, map, keyname, keylen, &keyname,
		&keylen, &result, &resultlen))
	      return ((struct hostent *)NULL);
	  }
	}

	if ((cp = index(result, '\n')))
		*cp = '\0';

	cp = strpbrk(result, " \t");
	*cp++ = '\0';
#if BSD >= 43 || defined(h_addr)	/* new-style hostent structure */
	h.h_addr_list = host_addrs;
#endif
	h.h_addr = hostaddr;
	*((u_long *)h.h_addr) = inet_addr(result);
	h.h_length = sizeof(u_long);
	h.h_addrtype = AF_INET;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	h.h_name = cp;
	q = h.h_aliases = host_aliases;
	cp = strpbrk(cp, " \t");
	if (cp != NULL)
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &host_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = strpbrk(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&h);
}
#endif /* NIS */

struct hostent *
_gethtbyaddr(const char *addr, int len, int type)
{
	register struct hostent *p;

	_sethtent(0);
	while (( p = _gethtent()))
		if (p->h_addrtype == type && !bcmp(p->h_addr, addr, len))
			break;
	_endhtent();
	return (p);
}

#ifdef RESOLVSORT
static void
addrsort( char **ap, int num )
{
	int i, j;
	char **p;
	short aval[MAXADDRS];
	int needsort = 0;

	p = ap;
	for (i = 0; i < num; i++, p++) {
	    for (j = 0 ; j < _res.nsort; j++)
		if (_res.sort_list[j].addr.s_addr ==
		    (((struct in_addr *)(*p))->s_addr & _res.sort_list[j].mask))
			break;
	    aval[i] = j;
	    if (needsort == 0 && i > 0 && j < aval[i-1])
		needsort = i;
	}
	if (!needsort)
	    return;

	while (needsort < num) {
	    for (j = needsort - 1; j >= 0; j--) {
		if (aval[j] > aval[j+1]) {
		    char *hp;

		    i = aval[j];
		    aval[j] = aval[j+1];
		    aval[j+1] = i;

		    hp = ap[j];
		    ap[j] = ap[j+1];
		    ap[j+1] = hp;

		} else
		    break;
	    }
	    needsort++;
	}
}
#endif
