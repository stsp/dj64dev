/*
 * Copyright (c) 1983 Regents of the University of California.
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


#include "i_priv.h"         /* RD: inetprivate.h -> i_priv.h */
#include <pwd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <resolv.h>

#include <lsck/lsck.h>      /* RD: Use configuration routines */
#include <lsck/domname.h>   /* RD: Use domain name functions  */
#include <lsck/ini.h>       /* RD: Use .ini functions         */

#if NLS
#include "nl_types.h"
#endif

#ifdef YP
#include <rpcsvc/ypclnt.h>
extern void setnetgrent(const char *);
extern void endnetgrent(void);
extern int getnetgrent(char **, char **, char **);
static char *nisdomain = NULL;
static int _checknetgrouphost(const char *, const char *, int);
static int _checknetgroupuser(const char *, const char *);
#endif

static char HEQUIVDB[1024];
static char name_buf[1024];

int
rresvport(alport)
	int *alport;
{
	struct sockaddr_in sin;
	int s;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return (-1);
	for (;;) {
		sin.sin_port = htons((u_short)*alport);
		if (bind(s, (struct sockaddr *)&sin, sizeof (sin)) >= 0)
			return (s);
		if (errno != EADDRINUSE) {
			(void) close(s);
			return (-1);
		}
		(*alport)--;
		if (*alport == IPPORT_RESERVED/2) {
			(void) close(s);
			errno = EAGAIN;		/* close */
			return (-1);
		}
	}
}

int
rcmd(ahost, rport, locuser, remuser, cmd, fd2p)
	char **ahost;
	unsigned short rport;
	const char *locuser, *remuser, *cmd;
	int *fd2p;
{
	int s, timo = 1;
#ifdef F_SETOWN
	pid_t pid;
#endif
#ifdef _POSIX_SOURCE
	sigset_t set, oset;
#else
/*  long oldmask; */
#endif
	struct sockaddr_in sin, from;
	char c;
	int lport = IPPORT_RESERVED - 1;
	struct hostent *hp;

#if NLS
	libc_nls_init();
#endif

#ifdef F_SETOWN
	pid = getpid();
#endif
	hp = gethostbyname(*ahost);
	if (hp == 0 || (int)sizeof (sin.sin_addr) < hp->h_length) {
#if NLS
		fprintf(stderr, "%s: %s\n", *ahost,
                              catgets(_libc_cat, HerrorListSet,
				      2, "unknown host"));
#else
		fprintf(stderr, "%s: unknown host\n", *ahost);
#endif
		return (-1);
	}
	strlcpy(name_buf, hp->h_name, sizeof(name_buf));
	*ahost = name_buf;
#ifdef SIGURG
#ifdef _POSIX_SOURCE
	sigemptyset (&set);
	sigaddset (&set, SIGURG);
	sigprocmask (SIG_BLOCK, &set, &oset);
#else
	oldmask = sigblock(sigmask(SIGURG));
#endif
#endif
	for (;;) {
		s = rresvport(&lport);
		if (s < 0) {
			if (errno == EAGAIN)
#if NLS
				fprintf(stderr, "socket: %s\n",
					catgets(_libc_cat, NetMiscSet,
						NetMiscAllPortsInUse,
						"All ports in use"));
#else
				fprintf(stderr, "socket: All ports in use\n");
#endif
			else
#if NLS
				perror(catgets(_libc_cat, NetMiscSet,
					       NetMiscRcmdSocket,
					       "rcmd: socket"));
#else
				perror("rcmd: socket");
#endif
#ifdef SIGURG
#ifdef _POSIX_SOURCE
			 sigprocmask (SIG_SETMASK, &oset,
				(sigset_t *)NULL);
#else
			sigsetmask(oldmask);
#endif
#endif
			return (-1);
		}
#ifdef F_SETOWN
		fcntl(s, F_SETOWN, pid);
#endif
		sin.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr_list[0], (caddr_t)&sin.sin_addr, hp->h_length);
		sin.sin_port = rport;
		if (connect(s, (struct sockaddr *)&sin, sizeof (sin)) >= 0)
			break;
		(void) close(s);
		if (errno == EADDRINUSE) {
			lport--;
			continue;
		}
		if (errno == ECONNREFUSED && timo <= 16) {
			sleep(timo);
			timo *= 2;
			continue;
		}
		if (hp->h_addr_list[1] != NULL) {
			int oerrno = errno;

			fprintf(stderr,
#if NLS
				"%s %s: ", catgets(_libc_cat, NetMiscSet,
						   NetMiscAllPortsInUse,
						   "connect to address"),
					   inet_ntoa(sin.sin_addr));

#else

			    "connect to address %s: ", inet_ntoa(sin.sin_addr));
#endif
			errno = oerrno;
			perror(0);
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0], (caddr_t)&sin.sin_addr,
			    hp->h_length);

#if NLS
			fprintf(stderr, catgets(_libc_cat, NetMiscSet,
						NetMiscTrying,
						"Trying %s...\n"),
#else
			fprintf(stderr,	"Trying %s...\n",
#endif
				inet_ntoa(sin.sin_addr));
			continue;
		}
		perror(hp->h_name);
#ifdef SIGURG
#ifdef _POSIX_SOURCE
		sigprocmask (SIG_SETMASK, &oset, (sigset_t *)NULL);
#else
		sigsetmask(oldmask);
#endif
#endif
		return (-1);
	}
	lport--;
	if (fd2p == 0) {
		write(s, "", 1);
		lport = 0;
	} else {
		char num[8];
		int s2 = rresvport(&lport), s3;

		/* Richard Dawe (libsocket): int -> size_t */
		size_t len = sizeof (from);

		if (s2 < 0)
			goto bad;
		listen(s2, 1);
		(void) sprintf(num, "%d", lport);
		if (write(s, num, strlen(num)+1) != (int)strlen(num)+1) {
#if NLS
			perror(catgets(_libc_cat, NetMiscSet,
				       NetMiscSettingUpStderr,
				       "write: setting up stderr"));
#else
			perror("write: setting up stderr");
#endif
			(void) close(s2);
			goto bad;
		}
		s3 = accept(s2, (struct sockaddr *)&from, &len);
		(void) close(s2);
		if (s3 < 0) {
#if NLS
			perror(catgets(_libc_cat, NetMiscSet,
				       NetMiscAccept,
				       "accept"));
#else
			perror("accept");
#endif
			lport = 0;
			goto bad;
		}
		*fd2p = s3;
		from.sin_port = ntohs((u_short)from.sin_port);
		if (from.sin_family != AF_INET ||
		    from.sin_port >= IPPORT_RESERVED) {
			fprintf(stderr,
#if NLS
				"%s\n",
				catgets(_libc_cat, NetMiscSet,
					NetMiscProtocolFailure,
					"socket: protocol failure in circuit setup."));
#else
			    "socket: protocol failure in circuit setup.\n");
#endif
			goto bad2;
		}
	}
	(void) write(s, locuser, strlen(locuser)+1);
	(void) write(s, remuser, strlen(remuser)+1);
	(void) write(s, cmd, strlen(cmd)+1);
	if (read(s, &c, 1) != 1) {
		perror(*ahost);
		goto bad2;
	}
	if (c != 0) {
		while (read(s, &c, 1) == 1) {
			(void) write(2, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad2;
	}
#ifdef SIGURG
#ifdef _POSIX_SOURCE
	sigprocmask (SIG_SETMASK, &oset, (sigset_t *)NULL);
#else
	sigsetmask(oldmask);
#endif
#endif
	return (s);
bad2:
	if (lport)
		(void) close(*fd2p);
bad:
	(void) close(s);
#ifdef SIGURG
#ifdef _POSIX_SOURCE
	sigprocmask (SIG_SETMASK, &oset, (sigset_t *)NULL);
#else
	sigsetmask(oldmask);
#endif
#endif
	return (-1);
}

int
ruserok(const char *rhost, int superuser, const char *ruser,
	const char *luser)
{
	FILE *hostf;
	char *fhost;
	int first = 1;
	register const char *sp;
	register char *p;
	int baselen = -1;
//	uid_t saveuid;
	char *x;

    /* RD: Use configuration routine now */
    /*if ( ( x = getenv ( "windir" ) ) == NULL ) HEQUIVDB[0] = 0;*/
    HEQUIVDB[0] = 0;
    x = __lsck_config_getfile();
    if (x != NULL) {
        GetPrivateProfileString("main", "hosts.equiv", NULL,
                                HEQUIVDB, sizeof(HEQUIVDB), x);
    }

//	saveuid = geteuid();
	sp = rhost;
	fhost = alloca (strlen (rhost) + 1);
	p = fhost;
	while (*sp) {
		if (*sp == '.') {
			if (baselen == -1)
				baselen = sp - rhost;
			*p++ = *sp++;
		} else {
			*p++ = isupper(*sp) ? tolower(*sp++) : *sp++;
		}
	}
	*p = '\0';
    hostf = superuser ? (FILE *)0 : fopen(HEQUIVDB, "rt");
again:
	if (hostf) {
		if (!_validuser(hostf, fhost, luser, ruser, baselen)) {
			(void) fclose(hostf);
/* IM: no point			seteuid(saveuid); */
			return(0);
		}
		(void) fclose(hostf);
	}
	if (first == 1) {
		struct stat sbuf;
		struct passwd *pwd;
		char *pbuf;

		first = 0;

        /* RD: Under DJGPP, this will probably fail, so ignore failure. */
        /*if ((pwd = getpwnam(luser)) == NULL) return(-1);*/
        pwd = getpwnam(luser);

        /* RD: Need to be more flexible here */
        /*pbuf = alloca (strlen (pwd->pw_dir) + sizeof ("/.rhosts") + 1);*/
        pbuf = malloc(PATH_MAX);
        if (pbuf == NULL) return(-1);
        pbuf[0] = '\0';

        /*
           RD: Look for ".rhosts" in various places:

           1. In the user's home directory, as ".rhosts".
           2. In the user's home directory, as "rhosts".
           3. In the user's home directory, as "_rhosts".
           4. In the place specified in the libsocket configuration file.
        */

        if (pwd != NULL) {
            /* .rhosts */
            strcpy(pbuf, pwd->pw_dir);
            strcat(pbuf, "/.rhosts");

            if (access(pbuf, R_OK) != 0) {  /* Can't find it! */
                /* Try rhosts */
                strcpy(pbuf, pwd->pw_dir);
                strcat(pbuf, "/rhosts");

                /* Can't find rhosts, so try _rhosts. */
                if (access(pbuf, R_OK) != 0) {
                    strcpy(pbuf, pwd->pw_dir);
                    strcat(pbuf, "/_rhosts");
                } pbuf[0] = '\0';

                /* Can't find it! */
                if (access(pbuf, R_OK) != 0) pbuf[0] = '\0';
            }
        }

        /* libsocket configuration */
        if (pbuf[0] == '\0') {
            x = __lsck_config_getfile();
            if (x != NULL) {
                GetPrivateProfileString("main", ".rhosts", NULL,
                                        pbuf, PATH_MAX, x);
            }
        }

/*        (void)strcpy(pbuf, pwd->pw_dir);
        (void)strcat(pbuf, "/.rhosts");*/
/* IM:		(void)seteuid(pwd->pw_uid); no point */

        if ((hostf = fopen(pbuf, "rt")) == NULL) {
/*			seteuid(saveuid); */
            free(pbuf);     /* RD: Stop leaks */
			return(-1);
		}
		(void)fstat(fileno(hostf), &sbuf);
		if (sbuf.st_uid && sbuf.st_uid != pwd->pw_uid) {
			fclose(hostf);
/*			seteuid(saveuid); */
            free(pbuf);     /* RD: Stop leaks */
			return(-1);
		}
        free(pbuf);     /* RD: Stop leaks */
		goto again;
	}
/*	seteuid(saveuid); */
	return (-1);
}

int
_validuser(FILE *hostf, const char *rhost, const char *luser,
	const char *ruser, int baselen)
{
	char *user;
	char ahost[MAXHOSTNAMELEN];
	register char *p;
#ifdef YP
    int hostvalid = 0;
    int uservalid = 0;
#endif

	while (fgets(ahost, sizeof (ahost), hostf)) {
		/* We need to get rid of all comments. */
		p = strchr (ahost, '#');
		if (p) *p = '\0';
		p = ahost;
		while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0') {
			*p = isupper(*p) ? tolower(*p) : *p;
			p++;
		}
		if (*p == ' ' || *p == '\t') {
			*p++ = '\0';
			while (*p == ' ' || *p == '\t')
				p++;
			user = p;
			while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0')
				p++;
		} else
			user = p;
		*p = '\0';
#ifdef YP
	uservalid = hostvalid = 0;
            /* disable host from -hostname entry */
        if ('-' == ahost[0] && '@' != ahost[1]
            && _checkhost(rhost, &ahost[1], baselen))
          hostvalid = 1;
            /* disable host from -@netgroup entry for host */
        if ('-' == ahost[0] && '@' == ahost[1] && '\0' != ahost[2]
            && _checknetgrouphost(rhost, &ahost[2], baselen))
          hostvalid = 1;
            /* disable user from -user entry */
        if ('\0' != *user && user[0] == '-' && user[1] != '@'
            && !strcmp(&user[1], ruser))
          uservalid = 1;
            /* disable user from -@netgroup entry for user */
        if ('\0' != *user && user[0] == '-' && user[1] == '@'
            && user[2] != '\0' && _checknetgroupuser(ruser, &user[2]))
          uservalid = 1;
	if (hostvalid && (uservalid || *user == '\0'))
	  return -1;

            /* enable host from +@netgroup entry for host */
        if ('+' == ahost[0] && '@' == ahost[1] && '\0' != ahost[2])
          hostvalid = _checknetgrouphost(rhost, &ahost[2], baselen);
        else
          hostvalid = _checkhost(rhost, ahost, baselen);
            /* enable user from +@netgroup entry for user */
	/* -@netgroup entry already gave uservalid=1 */
	if (user[0] == '+')
	  if (user[1] == '@' && user[2] != '\0')
	    uservalid = _checknetgroupuser(ruser, &user[2]);
	  else uservalid = !strcmp(&user[1], ruser);
	else if (user[0] == '-')
	  uservalid = -uservalid;
	else
	  uservalid = !strcmp(ruser, *user ? user : luser);

        if (hostvalid)
	  if (uservalid == 1)
	    return 0;
	else if (uservalid == -1)
	  return (-1);
#else
		if (_checkhost(rhost, ahost, baselen) &&
		    !strcmp(ruser, *user ? user : luser)) {
          return (0);
		}
#endif /* YP */
	}
	return (-1);
}

int
_checkhost(const char *rhost, const char *lhost, int len)
{
	static char ldomain[MAXHOSTNAMELEN + 1];
	static char *domainp = NULL;
	static int nodomain = 0;
	register char *cp;

	if (len == -1)
		return(!strcmp(rhost, lhost));
	if (strncmp(rhost, lhost, len))
		return(0);
	if (!strcmp(rhost, lhost))
		return(1);
	if (*(lhost + len) != '\0')
		return(0);
	if (nodomain)
		return(0);
	if (!domainp) {
		if (getdomainname(ldomain, sizeof(ldomain)) == -1) {
			nodomain = 1;
			return(0);
		}
		ldomain[MAXHOSTNAMELEN] = (char) 0;
		domainp = ldomain;
		for (cp = domainp; *cp; ++cp)
			if (isupper(*cp))
				*cp = tolower(*cp);
	}
	return(!strcmp(domainp, rhost + len +1));
}

#ifdef YP
static int
_checknetgrouphost(const char *rhost, const char *netgr, int baselen)
{
  char *host, *user, *domain;
  int status;

  if (NULL == nisdomain)
    yp_get_default_domain(&nisdomain);

  setnetgrent(netgr);
  while (1)
    {
      status = getnetgrent(&host, &user, &domain);

      if (0 == status)
        {
          endnetgrent();
          return 0;
        }

      if((domain == NULL || !strcmp(domain, nisdomain))
	 && (host == NULL || _checkhost(rhost, host, baselen)))
        {
          endnetgrent();
          return 1;
        }
    }
}

static int
_checknetgroupuser(const char *ruser, const char *netgr)
{
  char *host, *user, *domain;
  int status;

  if (NULL == nisdomain)
    yp_get_default_domain(&nisdomain);

  setnetgrent(netgr);
  while (1)
    {
      status = getnetgrent(&host, &user, &domain);

      if (0 == status)
        {
          endnetgrent();
          return 0;
        }

      if((domain == NULL || 0 == strcmp(domain, nisdomain))
	 && (user == NULL || 0 == strcmp(ruser, user)))
        {
          endnetgrent();
          return 1;
        }
    }
}
#endif /* YP */
