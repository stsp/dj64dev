#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <lsck/bsdtypes.h>

extern void _sethtent(int);
extern void _endhtent(void);
extern struct hostent *_gethtent(void);
extern struct hostent *_gethtbyname(const char *);
extern struct hostent *_gethtbyaddr(const char *, int, int);
extern int _validuser(FILE *, const char *, const char *, const char *, int);
extern int _checkhost(const char *, const char *, int);
extern void putlong(u_long l, u_char *);
extern void putshort(u_short l, u_char *);
extern u_int32_t _getlong(register const u_char *);
extern u_int16_t _getshort(register const u_char *);
extern void p_query(char *);
extern void fp_query(char *, FILE *);
extern char *p_cdname(char *, char *, FILE *);
extern char *p_rr(char *, char *, FILE *);
extern char *p_type(int);
extern char * p_class(int);
extern char *p_time(u_long);
extern char * hostalias(const char *);
extern void sethostfile(char *);
extern void _res_close (void);
extern void ruserpass(const char *, const char **, const char **);
