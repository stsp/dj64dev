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

/*
   Some modifications by Richard Dawe (RD) for use with libsocket. Some
   modifications made by RD based on Phillipe Hanrigou's (PH) code. Comments
   are indicated by "RD:" or "PH:" respectively.
*/

#include "i_priv.h"     /* RD: inetprivate.h -> i_priv.h */
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>

/* RD: Includes */
#include <limits.h>
#include <unistd.h>
#include <time.h>

#include <netdb.h>

#include <lsck/lsck.h>
#include <lsck/ini.h>
/* End RD */

/* Start RD: utmp def's */
/* This uses the same order as DJGPP's tmpnam() to find the temporary dir. */
static char *fake_path_utmp (void)
{
	char *p = NULL;
	p = getenv("TMPDIR");
	if (p == NULL) p = getenv("TEMP");
	if (p == NULL) p = getenv("TMP");
	return(p);
}

#define _PATH_UTMP	( fake_path_utmp() )

#define UT_LINESIZE	12
#define UT_NAMESIZE	8
#define UT_HOSTSIZE	16

struct utmp
{
	short	ut_type;	/* type of login */
	pid_t	ut_pid;		/* pid of login-process */
	char	ut_line[UT_LINESIZE];	/* devicename of tty -"/dev/", null-term */
	char	ut_id[4];	/* inittab id */
	time_t	ut_time;	/* login time */
	char	ut_user[UT_NAMESIZE];	/* username, not null-term */
	char	ut_host[UT_HOSTSIZE];	/* hostname for remote login... */
	long	ut_addr;	/* IP addr of remote host */
};
/* End RD */

extern char *__libc_secure_getenv(const char *);

static FILE *cfile;

static int token( void );
static void rnetrc(const char *, const char **, const char ** );
static char *renvlook( const char * );
static void renv( const char *, const char **, const char ** );
static void mkpwclear( char *, char, const char * );
static char *mkenvkey( char );
static void sreverse( register char *, register char * );
static struct utmp *getutmp(char * );
static void blkencrypt(char *, int );
static void nbssetkey( char * );
static void enblknot( char *, char * );
static char *deblkclr( char * );
static void enblkclr( char *, char * );
static char *nbs8decrypt( char *, char * );
static void mkpwclear( char *, char, const char * );
static char *mkenvkey( char );
static void sreverse( register char *, register char * );
static struct utmp *getutmp( char * );
static void blkencrypt( char *, int );
static void nbssetkey( char * );
static void enblknot( char *, char * );
static char *deblkclr( char * );
static void enblkclr( char *, char * );
static char *nbs8decrypt( char *, char * );
static char *nbsdecrypt( const char *, char *, char * );

#if NLS
#include "nl_types.h"
#endif

void
ruserpass(const char *host, const char **aname, const char **apass)
{
	static char namebuf [256];
	struct hostent *hp;
	char    name[256]; /* a temp name buffer to avoid overlayyed */

#if NLS
	libc_nls_init();
#endif
	strncpy(name, host, sizeof(name) - 1);

	if ((hp = gethostbyname (name)))
		strncpy(name,hp->h_name, sizeof(name) - 1);
	renv(name, aname, apass);
	if (*aname == 0 || *apass == 0)
		rnetrc(name, aname, apass); /*rnetrc would call gethostbyname */
	if (*aname == 0) {
		*aname = getlogin();
		if (*aname == NULL) {
			struct passwd *pp = getpwuid(getuid());
			if (pp != NULL)
				*aname = pp->pw_name;
		}
#if NLS
		printf("%s (%s:%s): ",
		       catgets(_libc_cat, NetMiscSet, NetMiscName, "Name"),
		       host, *aname);
#else
		printf("Name (%s:%s): ", host, *aname);
#endif
		fflush(stdout);
		if (read(2, namebuf, sizeof (namebuf)) <= 0) {
			perror ("read");
			exit(1);
		}
		if (namebuf [0] != '\n') {
			char *ptr;
			*aname = namebuf;
			namebuf [sizeof (namebuf) - 1] = '0';
			if ((ptr = index(namebuf, '\n')))
				*ptr = 0;
		}
	}
	if (*aname && *apass == 0) {
#if NLS
		printf("%s (%s:%s): ",
		       catgets(_libc_cat, NetMiscSet, NetMiscPassword, "Password"),
		       host, *aname);
#else
		printf("Password (%s:%s): ", host, *aname);
#endif
		fflush(stdout);
		*apass = getpass("");
	}
}

static void
renv(const char *host, const char **aname, const char **apass)
{
	register char *cp;
	char *comma;

	cp = renvlook(host);
	if (cp == NULL)
		return;
	if (!isalpha(cp[0]))
		return;
	comma = index(cp, ',');
	if (comma == 0)
		return;
	if (*aname == 0) {
		char *_aname = malloc(comma - cp + 1);
		strncpy(_aname, cp, comma - cp);
		*aname = _aname;
	} else
		if (strncmp(*aname, cp, comma - cp))
			return;
	comma++;
	cp = malloc(strlen(comma)+1);
	strcpy(cp, comma);
	*apass = malloc(16);
	mkpwclear(cp, host[0], *apass);
}

//#if !defined(HAVE_GNU_LD) && !defined (__ELF__)
#define	__environ	environ
//#endif

static char *
renvlook(const char *host)
{
	register char *cp, **env;
	extern char **__environ;

	for (env = __environ; *env != NULL; env++)
		if (!strncmp(*env, "MACH", 4)) {
			cp = index(*env, '=');
			if (cp == 0)
				continue;
			if (strncmp(*env+4, host, cp-(*env+4)))
				continue;
			return (cp+1);
		}
	return (NULL);
}

#define	DEFAULT	1
#define	LOGIN	2
#define	PASSWD	3
#define	NOTIFY	4
#define	WRITE	5
#define	YES	6
#define	NO	7
#define	COMMAND	8
#define	FORCE	9
#define	ID	10
#define	MACHINE	11

static char tokval[100];

static struct toktab {
	const char *tokstr;
	int tval;
} toktab[]= {
	{ "default",	DEFAULT },
	{ "login",	LOGIN },
	{ "password",	PASSWD },
	{ "notify",	NOTIFY },
	{ "write",	WRITE },
	{ "yes",		YES },
	{ "y",		YES },
	{ "no",		NO },
	{ "n",		NO },
	{ "command",	COMMAND },
	{ "force",	FORCE },
	{ "machine",	MACHINE },
	{ 0,		0 }
};

static void
rnetrc(const char *host, const char **aname, const char **apass)
{
	char *hdir, *buf;
	int t;
	struct stat stb;
	struct hostent *hp;
    char *cfg = NULL;   /* RD: libsocket's config file */

#if NLS
	libc_nls_init();
#endif
	/*
	 * Use __libc_secure_getenv -- otherwise you can probably read any
	 * file on the system by setting $HOME to /tmp/foo and linking
	 * /tmp/foo/.netrc, if you can find a setuid program using this code.
	 * Also, if we can't get HOME, fail instead of trying ".", which is
	 * no improvement. This really should call getpwuid(getuid()).
	 */

    /* RD: We need to look a bit harder than this... */
    /*hdir = __libc_secure_getenv("HOME");
    if (hdir == NULL) {*/
		/* hdir = "."; */
		/* NO! return instead */
        /*return;
	}
	buf = alloca (strlen (hdir) + sizeof ("/.netrc") + 1);
    (void)sprintf(buf, "%s/.netrc", hdir);*/

    /*
       Try to find the netrc file in the following way:

       1. Home directory, called ".netrc".
       2. Home directory, called "netrc".
       3. Home directory, called "_netrc", as suggested by Phillipe Hanrigou.
       4. As specified by ".netrc" in the libsocket configuration file.
    */

    /* Look in the home directory for .netrc, netrc */
    buf = malloc(PATH_MAX);
    if (buf == NULL) return;

    hdir = __libc_secure_getenv("HOME");
    if (hdir == NULL) return;
    sprintf(buf, "%s/.netrc", hdir);
    if (access(buf, R_OK) != 0) sprintf(buf, "%s/netrc", hdir);
    if (access(buf, R_OK) != 0) sprintf(buf, "%s/_netrc", hdir);

    /* OK, now try looking at libsocket's config file */
    if (access(buf, R_OK) != 0) {
        cfg = __lsck_config_getfile();
        if (cfg == NULL) { free(buf); return; }
        GetPrivateProfileString("main", ".netrc", NULL, buf, sizeof(buf), cfg);
    }
    /* End RD */

    cfile = fopen(buf, "rt");
	if (cfile == NULL) {
		if (errno != ENOENT)
			perror(buf);
        free(buf);  /* RD: Don't want a memory leak! */
		return;
	}
next:
	while ((t = token())) switch(t) {

	case DEFAULT:
		(void) token();
		continue;

	case MACHINE:
		if (token() != ID)
			continue;
		if((hp = gethostbyname (tokval)))
		{
			if (strcmp(host, hp->h_name))
				continue;
		}
		else
			if (strcmp(host, tokval))
				continue;
		while ((t = token()) && t != MACHINE) switch(t) {

		case LOGIN:
			if (token()) {
				if (*aname == 0) {
					char *_aname = malloc(strlen(tokval) + 1);
					strcpy(_aname, tokval);
					*aname = _aname;
				} else {
					if (strcmp(*aname, tokval))
						goto next;
				}
			}
			break;
		case PASSWD:
            /* RD: Phillipe Hanrigou patch here! */
            if (   ((*aname == NULL) || (strcmp(*aname, "anonymous")))
                && (fstat(fileno(cfile), &stb) >= 0)
                && ((stb.st_mode & 077) != 0)
               ) {
#if NLS
	fprintf(stderr, "%s\n", catgets(_libc_cat, NetMiscSet,
				NetMiscNetrcWrongPasswordMode,
				"Error - .netrc file not correct mode.\n\
				 Remove password or correct mode."));
#else
	fprintf(stderr, "Error - .netrc file not correct mode.\n");
	fprintf(stderr, "Remove password or correct mode.\n");
#endif
                /* RD: Too harsh for DJGPP */
                /*exit(1);*/

                /*
                 * Philippe HANRIGOU - 11/12/98
                 *
                 * Dos and Windows95 are not really famous as multiuser
                 * platforms. ;-) Let's allow .netrc to be readable by
                 * others and just display a warning.
                 */
                /* RD: Multi-line string used now. */
                fprintf(stderr, "\nWARNING: .netrc file is readable by "
                                "others and contains password(s) in \nplain "
                                "text. Please note this is a BIG SECURITY "
                                "HOLE if somebody else has\naccess to "
                                "your computer. Maybe it would be a good "
                                "idea to remove\npassword(s).\n\n");
			}

			if (token() && *apass == 0) {
				char *_apass = malloc(strlen(tokval) + 1);
				strcpy(_apass, tokval);
				*apass = _apass;
			}
			break;
		case COMMAND:
		case NOTIFY:
		case WRITE:
		case FORCE:
			(void) token();
			break;
		default:
#if NLS
	fprintf(stderr, "%s %s\n",
		catgets(_libc_cat, NetMiscSet,
			NetMiscUnknownNetrcOption,
			"Unknown .netrc option"),
		tokval);
#else
	fprintf(stderr, "Unknown .netrc option %s\n", tokval);
#endif
			break;
		}
		goto done;
	}
done:
	fclose(cfile);
    free(buf);      /* RD: Don't want a memory leak! */
}

static int
token( void )
{
	char *cp;
	int c;
	struct toktab *t;

	if (feof(cfile))
		return (0);
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(_REENTRANT)
#undef getc
#define getc	getc_unlocked
	flockfile (cfile);
#endif
	while ((c = getc(cfile)) != EOF &&
	    (c == '\n' || c == '\t' || c == ' ' || c == ','))
		continue;
	if (c == EOF)
		return (0);
	cp = tokval;
	if (c == '"') {
		while ((c = getc(cfile)) != EOF && c != '"') {
			if (c == '\\')
				c = getc(cfile);
			*cp++ = c;
		}
	} else {
		*cp++ = c;
		while ((c = getc(cfile)) != EOF
		    && c != '\n' && c != '\t' && c != ' ' && c != ',') {
			if (c == '\\')
				c = getc(cfile);
			*cp++ = c;
		}
	}
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(_REENTRANT)
	funlockfile (cfile);
#endif
	*cp = 0;
	if (tokval[0] == 0)
		return (0);
	for (t = toktab; t->tokstr; t++)
		if (!strcmp(t->tokstr, tokval))
			return (t->tval);
	return (ID);
}

/* rest is nbs.c stolen from berknet */

#if 0
static char *nbsencrypt(char *, char *, char * );
static char *nbs8encrypt(char *, char * );
static char *deblknot(char * );
#endif

static char	E[48];

/*
 * The E bit-selection table.
 */
static char	e[] = {
	32, 1, 2, 3, 4, 5,
	 4, 5, 6, 7, 8, 9,
	 8, 9,10,11,12,13,
	12,13,14,15,16,17,
	16,17,18,19,20,21,
	20,21,22,23,24,25,
	24,25,26,27,28,29,
	28,29,30,31,32, 1,
};

#if 0
/*
 * Warning -- do not enable any of this without checking it carefully
 * for buffer overflows.
 */
static
char *nbsencrypt( char *str, char *key, char *result )
{
	static char buf[20],oldbuf[20];
	register int j;
	result[0] = 0;
	strcpy(oldbuf,key);
	while(*str){
		for(j=0;j<10;j++)buf[j] = 0;
		for(j=0;j<8 && *str;j++)buf[j] = *str++;
		strcat(result,nbs8encrypt(buf,oldbuf));
		strcat(result,"$");
		strcpy(oldbuf,buf);
		}
	return(result);
}

#endif	/* if 0 */

static
char *nbsdecrypt( const char *_cpt, char *key, char *result )
{
	char *s;
	char c,oldbuf[20],__cpt[128],*cpt=__cpt;
	result[0] = 0;
	strcpy(oldbuf,key);
	strlcpy(cpt, _cpt, sizeof(cpt));
	while(*cpt){
		for(s = cpt;*s && *s != '$';s++);
		c = *s;
		*s = 0;
		strcpy(oldbuf,nbs8decrypt(cpt,oldbuf));
		strcat(result,oldbuf);
		if(c == 0)break;
		cpt = s + 1;
		}
	return(result);
}

#if 0
static
char *nbs8encrypt( char *str, char *key )
{
	static char keyblk[100], blk[100];
	register int i;

	enblkclr(keyblk,key);
	nbssetkey(keyblk);

	for(i=0;i<48;i++) E[i] = e[i];
	enblkclr(blk,str);
	blkencrypt(blk,0);			/* forward dir */

	return(deblknot(blk));
}

#endif	/* if 0 */

static
char *nbs8decrypt( char *crp, char *key )
{
	static char keyblk[100], blk[100];
	register int i;

	enblkclr(keyblk,key);
	nbssetkey(keyblk);

	for(i=0;i<48;i++) E[i] = e[i];
	enblknot(blk,crp);
	blkencrypt(blk,1);			/* backward dir */

	return(deblkclr(blk));
}

static void
enblkclr( char *blk, char *str )
	/* ignores top bit of chars in string str */
{
	register int i,j;
	char c;
	for(i=0;i<70;i++)blk[i] = 0;
	for(i=0; (c= *str) && i<64; str++){
		for(j=0; j<7; j++, i++)
			blk[i] = (c>>(6-j)) & 01;
		i++;
		}
}

static
char *deblkclr( char *blk )
{
	register int i,j;
	char c;
	static char iobuf[30];
	for(i=0; i<10; i++){
		c = 0;
		for(j=0; j<7; j++){
			c <<= 1;
			c |= blk[8*i+j];
			}
		iobuf[i] = c;
	}
	iobuf[i] = 0;
	return(iobuf);
}

static void
enblknot( char *blk, char *crp )
{
	register int i,j;
	char c;
	for(i=0;i<70;i++)blk[i] = 0;
	for(i=0; (c= *crp) && i<64; crp++){
		if(c>'Z') c -= 6;
		if(c>'9') c -= 7;
		c -= '.';
		for(j=0; j<6; j++, i++)
			blk[i] = (c>>(5-j)) & 01;
		}
}

#if 0
static
char *deblknot( char *blk )
{
	register int i,j;
	char c;
	static char iobuf[30];
	for(i=0; i<11; i++){
		c = 0;
		for(j=0; j<6; j++){
			c <<= 1;
			c |= blk[6*i+j];
			}
		c += '.';
		if(c > '9')c += 7;
		if(c > 'Z')c += 6;
		iobuf[i] = c;
	}
	iobuf[i] = 0;
	return(iobuf);
}
#endif


/*
 * This program implements the
 * Proposed Federal Information Processing
 *  Data Encryption Standard.
 * See Federal Register, March 17, 1975 (40FR12134)
 */

/*
 * Initial permutation,
 */
static	char	IP[] = {
	58,50,42,34,26,18,10, 2,
	60,52,44,36,28,20,12, 4,
	62,54,46,38,30,22,14, 6,
	64,56,48,40,32,24,16, 8,
	57,49,41,33,25,17, 9, 1,
	59,51,43,35,27,19,11, 3,
	61,53,45,37,29,21,13, 5,
	63,55,47,39,31,23,15, 7,
};

/*
 * Final permutation, FP = IP^(-1)
 */
static	char	FP[] = {
	40, 8,48,16,56,24,64,32,
	39, 7,47,15,55,23,63,31,
	38, 6,46,14,54,22,62,30,
	37, 5,45,13,53,21,61,29,
	36, 4,44,12,52,20,60,28,
	35, 3,43,11,51,19,59,27,
	34, 2,42,10,50,18,58,26,
	33, 1,41, 9,49,17,57,25,
};

/*
 * Permuted-choice 1 from the key bits
 * to yield C and D.
 * Note that bits 8,16... are left out:
 * They are intended for a parity check.
 */
static	char	PC1_C[] = {
	57,49,41,33,25,17, 9,
	 1,58,50,42,34,26,18,
	10, 2,59,51,43,35,27,
	19,11, 3,60,52,44,36,
};

static	char	PC1_D[] = {
	63,55,47,39,31,23,15,
	 7,62,54,46,38,30,22,
	14, 6,61,53,45,37,29,
	21,13, 5,28,20,12, 4,
};

/*
 * Sequence of shifts used for the key schedule.
*/
static	char	shifts[] = {
	1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1,
};

/*
 * Permuted-choice 2, to pick out the bits from
 * the CD array that generate the key schedule.
 */
static	char	PC2_C[] = {
	14,17,11,24, 1, 5,
	 3,28,15, 6,21,10,
	23,19,12, 4,26, 8,
	16, 7,27,20,13, 2,
};

static	char	PC2_D[] = {
	41,52,31,37,47,55,
	30,40,51,45,33,48,
	44,49,39,56,34,53,
	46,42,50,36,29,32,
};

/*
 * The C and D arrays used to calculate the key schedule.
 */

static	char	C[28];
static	char	D[28];
/*
 * The key schedule.
 * Generated from the key.
 */
static	char	KS[16][48];

/*
 * Set up the key schedule from the key.
 */

static void
nbssetkey( char *key )
{
	register int i, j, k;
	int t;

	/*
	 * First, generate C and D by permuting
	 * the key.  The low order bit of each
	 * 8-bit char is not used, so C and D are only 28
	 * bits apiece.
	 */
	for (i=0; i<28; i++) {
		C[i] = key[PC1_C[i]-1];
		D[i] = key[PC1_D[i]-1];
	}
	/*
	 * To generate Ki, rotate C and D according
	 * to schedule and pick up a permutation
	 * using PC2.
	 */
	for (i=0; i<16; i++) {
		/*
		 * rotate.
		 */
		for (k=0; k<shifts[i]; k++) {
			t = C[0];
			for (j=0; j<28-1; j++)
				C[j] = C[j+1];
			C[27] = t;
			t = D[0];
			for (j=0; j<28-1; j++)
				D[j] = D[j+1];
			D[27] = t;
		}
		/*
		 * get Ki. Note C and D are concatenated.
		 */
		for (j=0; j<24; j++) {
			KS[i][j] = C[PC2_C[j]-1];
			KS[i][j+24] = D[PC2_D[j]-28-1];
		}
	}
}


/*
 * The 8 selection functions.
 * For some reason, they give a 0-origin
 * index, unlike everything else.
 */
static	char	S[8][64] = {
	{ 14, 4,13, 1, 2,15,11, 8, 3,10, 6,12, 5, 9, 0, 7,
	 0,15, 7, 4,14, 2,13, 1,10, 6,12,11, 9, 5, 3, 8,
	 4, 1,14, 8,13, 6, 2,11,15,12, 9, 7, 3,10, 5, 0,
	15,12, 8, 2, 4, 9, 1, 7, 5,11, 3,14,10, 0, 6,13 },

	{ 15, 1, 8,14, 6,11, 3, 4, 9, 7, 2,13,12, 0, 5,10,
	 3,13, 4, 7,15, 2, 8,14,12, 0, 1,10, 6, 9,11, 5,
	 0,14, 7,11,10, 4,13, 1, 5, 8,12, 6, 9, 3, 2,15,
	13, 8,10, 1, 3,15, 4, 2,11, 6, 7,12, 0, 5,14, 9 },

	{ 10, 0, 9,14, 6, 3,15, 5, 1,13,12, 7,11, 4, 2, 8,
	13, 7, 0, 9, 3, 4, 6,10, 2, 8, 5,14,12,11,15, 1,
	13, 6, 4, 9, 8,15, 3, 0,11, 1, 2,12, 5,10,14, 7,
	 1,10,13, 0, 6, 9, 8, 7, 4,15,14, 3,11, 5, 2,12 },

	{ 7,13,14, 3, 0, 6, 9,10, 1, 2, 8, 5,11,12, 4,15,
	13, 8,11, 5, 6,15, 0, 3, 4, 7, 2,12, 1,10,14, 9,
	10, 6, 9, 0,12,11, 7,13,15, 1, 3,14, 5, 2, 8, 4,
	 3,15, 0, 6,10, 1,13, 8, 9, 4, 5,11,12, 7, 2,14 },

	{ 2,12, 4, 1, 7,10,11, 6, 8, 5, 3,15,13, 0,14, 9,
	14,11, 2,12, 4, 7,13, 1, 5, 0,15,10, 3, 9, 8, 6,
	 4, 2, 1,11,10,13, 7, 8,15, 9,12, 5, 6, 3, 0,14,
	11, 8,12, 7, 1,14, 2,13, 6,15, 0, 9,10, 4, 5, 3 },

	{12, 1,10,15, 9, 2, 6, 8, 0,13, 3, 4,14, 7, 5,11,
	10,15, 4, 2, 7,12, 9, 5, 6, 1,13,14, 0,11, 3, 8,
	 9,14,15, 5, 2, 8,12, 3, 7, 0, 4,10, 1,13,11, 6,
	 4, 3, 2,12, 9, 5,15,10,11,14, 1, 7, 6, 0, 8,13 },

	{ 4,11, 2,14,15, 0, 8,13, 3,12, 9, 7, 5,10, 6, 1,
	13, 0,11, 7, 4, 9, 1,10,14, 3, 5,12, 2,15, 8, 6,
	 1, 4,11,13,12, 3, 7,14,10,15, 6, 8, 0, 5, 9, 2,
	 6,11,13, 8, 1, 4,10, 7, 9, 5, 0,15,14, 2, 3,12 },

	{13, 2, 8, 4, 6,15,11, 1,10, 9, 3,14, 5, 0,12, 7,
	 1,15,13, 8,10, 3, 7, 4,12, 5, 6,11, 0,14, 9, 2,
	 7,11, 4, 1, 9,12,14, 2, 0, 6,10,13,15, 3, 5, 8,
	 2, 1,14, 7, 4,10, 8,13,15,12, 9, 0, 3, 5, 6,11 },
};

/*
 * P is a permutation on the selected combination
 * of the current L and key.
 */
static	char	P[] = {
	16, 7,20,21,
	29,12,28,17,
	 1,15,23,26,
	 5,18,31,10,
	 2, 8,24,14,
	32,27, 3, 9,
	19,13,30, 6,
	22,11, 4,25,
};

/*
 * The current block, divided into 2 halves.
 */
static	char	L[32], R[32];
static	char	tempL[32];
static	char	f[32];

/*
 * The combination of the key and the input, before selection.
 */
static	char	preS[48];

/*
 * The payoff: encrypt a block.
 */

static void
blkencrypt(char *block, int edflag)
{
	int i, ii;
	register int t, j, k;

	/*
	 * First, permute the bits in the input
	 */
	for (j=0; j<64; j++)
		L[j] = block[IP[j]-1];
	/*
	 * Perform an encryption operation 16 times.
	 */
	for (ii=0; ii<16; ii++) {
		/*
		 * Set direction
		 */
		if (edflag)
			i = 15-ii;
		else
			i = ii;
		/*
		 * Save the R array,
		 * which will be the new L.
		 */
		for (j=0; j<32; j++)
			tempL[j] = R[j];
		/*
		 * Expand R to 48 bits using the E selector;
		 * exclusive-or with the current key bits.
		 */
		for (j=0; j<48; j++)
			preS[j] = R[E[j]-1] ^ KS[i][j];
		/*
		 * The pre-select bits are now considered
		 * in 8 groups of 6 bits each.
		 * The 8 selection functions map these
		 * 6-bit quantities into 4-bit quantities
		 * and the results permuted
		 * to make an f(R, K).
		 * The indexing into the selection functions
		 * is peculiar; it could be simplified by
		 * rewriting the tables.
		 */
		for (j=0; j<8; j++) {
			t = 6*j;
			k = S[j][(preS[t+0]<<5)+
				(preS[t+1]<<3)+
				(preS[t+2]<<2)+
				(preS[t+3]<<1)+
				(preS[t+4]<<0)+
				(preS[t+5]<<4)];
			t = 4*j;
			f[t+0] = (k>>3)&01;
			f[t+1] = (k>>2)&01;
			f[t+2] = (k>>1)&01;
			f[t+3] = (k>>0)&01;
		}
		/*
		 * The new R is L ^ f(R, K).
		 * The f here has to be permuted first, though.
		 */
		for (j=0; j<32; j++)
			R[j] = L[j] ^ f[P[j]-1];
		/*
		 * Finally, the new L (the original R)
		 * is copied back.
		 */
		for (j=0; j<32; j++)
			L[j] = tempL[j];
	}
	/*
	 * The output L and R are reversed.
	 */
	for (j=0; j<32; j++) {
		t = L[j];
		L[j] = R[j];
		R[j] = t;
	}
	/*
	 * The final output
	 * gets the inverse permutation of the very original.
	 */
	for (j=0; j<64; j++)
		block[j] = L[FP[j]-1];
}
/*
	getutmp()
	return a pointer to the system utmp structure associated with
	terminal sttyname, e.g. "/dev/tty3"
	Is version independent-- will work on v6 systems
	return NULL if error
*/
static
struct utmp *getutmp(char *sttyname)
{
	static struct utmp utmpstr;
	FILE *fdutmp;

	if(sttyname == NULL || sttyname[0] == 0)return(NULL);

    fdutmp = fopen(_PATH_UTMP,"rt");
	if(fdutmp == NULL)return(NULL);

	while(fread(&utmpstr,1,sizeof utmpstr,fdutmp) == sizeof utmpstr)
		if(strcmp(utmpstr.ut_line,sttyname+5) == 0){
			fclose(fdutmp);
			return(&utmpstr);
		}
	fclose(fdutmp);
	return(NULL);
}

static void
sreverse( register char *sto, register char *sfrom )
{
	register int i;

	i = strlen(sfrom);
	while (i >= 0)
		*sto++ = sfrom[i--];
}

static
char *mkenvkey(char mch)
{
	static char skey[40];
	register struct utmp *putmp;
	char stemp[40], stemp1[40];
	/* FUTURE: remove the limit on this size */
	char sttyname[30];
	register char *sk,*p;

	if (isatty(2))
		strncpy(sttyname,ttyname(2), sizeof(sttyname));
	else if (isatty(0))
		strncpy(sttyname,ttyname(0), sizeof(sttyname));
	else if (isatty(1))
		strncpy(sttyname,ttyname(1), sizeof(sttyname));
	else
		return (NULL);
	sttyname[sizeof(sttyname)-1] = 0;
	putmp = getutmp(sttyname);
	if (putmp == NULL)
		return (NULL);
	sk = skey;
	p = putmp->ut_line;
	while (*p)
		*sk++ = *p++;
	*sk++ = mch;
	(void)sprintf(stemp, "%ld", (long) putmp->ut_time);
	sreverse(stemp1, stemp);
	p = stemp1;
	while (*p)
		*sk++ = *p++;
	*sk = 0;
	return (skey);
}

#if 0
static void
mkpwunclear( char *spasswd, char mch, char *sencpasswd )
{
	register char *skey;

	if (spasswd[0] == 0) {
		sencpasswd[0] = 0;
		return;
	}
	skey = mkenvkey(mch);
	if (skey == NULL) {
		fprintf(stderr, "Can't make key\n");
		exit(1);
	}
	nbsencrypt(spasswd, skey, sencpasswd);
}

#endif

static void
mkpwclear( char *spasswd, char mch, const char *sencpasswd )
{
	register char *skey;

	if (sencpasswd[0] == 0) {
		spasswd[0] = 0;
		return;
	}
	skey = mkenvkey(mch);
	if (skey == NULL) {
		fprintf(stderr, "Can't make key\n");
		exit(1);
	}
	nbsdecrypt(sencpasswd, skey, spasswd);
}
