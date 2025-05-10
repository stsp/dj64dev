/*
 * i_nsap.c
 * Copyright unknown - presumably this is part of the Linux networking code.
 *
 * This file is part of libsocket.
 * libsocket Copyright 1997, 1998 by Indrek Mandre
 * libsocket Copyright 1997-2000 by Richard Dawe
 */

/* RD: inetprivate.h -> i_priv.h */
#include "i_priv.h"

char *inet_nsap_ntoa( int, register const u_char *, register char * );
u_int inet_nsap_addr( const char *, u_char *, int );
static char xtob( register int );

#if !defined(isxdigit)	/* XXX - could be a function */
static int
isxdigit(c)
	register int c;
{
	return ((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F'));
}
#endif	/* !isxdigit */

static char
xtob( register int c )
{
	return (c - (((c >= '0') && (c <= '9')) ? '0' : '7'));
}

u_int
inet_nsap_addr( const char *ascii, u_char *binary, int maxlen )
{
	register u_char c, nib;
/* 	u_char *start = binary; */
	int len = 0;

	while ((c = *ascii++) != '\0' && len < maxlen) {
		if (c == '.' || c == '+' || c == '/')
			continue;
		if (!isascii(c))
			return (0);
		if (islower(c))
			c = toupper(c);
		if (isxdigit(c)) {
			nib = xtob(c);
			if (( c = *ascii++ )) {
				c = toupper(c);
				if (isxdigit(c)) {
					*binary++ = (nib << 4) | xtob(c);
					len++;
				} else
					return (0);
			}
			else
				return (0);
		}
		else
			return (0);
	}
	return (len);
}

char *
inet_nsap_ntoa( int binlen, register const u_char *binary,
					register char *ascii )
{
	register int nib;
	int i;
	static char tmpbuf[255*3];
	char *start;

	if (ascii)
		start = ascii;
	else {
		ascii = tmpbuf;
		start = tmpbuf;
	}

	if (binlen > 255)
		binlen = 255;

	for (i = 0; i < binlen; i++) {
		nib = *binary >> 4;
		*ascii++ = nib + (nib < 10 ? '0' : '7');
		nib = *binary++ & 0x0f;
		*ascii++ = nib + (nib < 10 ? '0' : '7');
		if (((i % 2) == 0 && (i + 1) < binlen))
			*ascii++ = '.';
	}
	*ascii = '\0';
	return (start);
}
