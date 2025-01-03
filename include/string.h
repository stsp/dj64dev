/* Copyright (C) 2017 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2013 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2002 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2000 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_string_h_
#define __dj_include_string_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#include <sys/djtypes.h>

/* Some programs think they know better... */
#undef NULL
#if (__GNUC__ >= 4) && defined(__cplusplus)
#  define NULL          __null
#elif defined(__cplusplus)
#  define NULL          0
#else
#  define NULL          ((void*)0)
#endif

#ifndef _SIZE_T
__DJ_size_t
#define _SIZE_T
#endif

void *	memchr(const void *_s, int _c, size_t _n);
int	memcmp(const void *_s1, const void *_s2, size_t _n);
void *	memcpy(void *  _dest, const void *  _src, size_t _n);
void *	memmove(void *_s1, const void *_s2, size_t _n);
void *	memset(void *_s, int _c, size_t _n);
char *	strcat(char *  _s1, const char *  _s2);
char *	strchr(const char *_s, int _c);
int	strcmp(const char *_s1, const char *_s2);
int	strcoll(const char *_s1, const char *_s2);
char *	strcpy(char *  _s1, const char *  _s2);
size_t	strcspn(const char *_s1, const char *_s2);
char *	strerror(int _errcode);
size_t	strlen(const char *_s);
char *	strncat(char *  _s1, const char *  _s2, size_t _n);
int	strncmp(const char *_s1, const char *_s2, size_t _n);
char *	strncpy(char *  _s1, const char *  _s2, size_t _n);
char *	strpbrk(const char *_s1, const char *_s2);
char *	strrchr(const char *_s, int _c);
size_t	strspn(const char *_s1, const char *_s2);
char *	strstr(const char *_s1, const char *_s2);
char *	strtok(char *  _s1, const char *  _s2);
size_t	strxfrm(char *  _s1, const char *  _s2, size_t _n);

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined(__STRICT_ANSI__) || defined(__cplusplus)

#endif /* (__STDC_VERSION__ >= 199901L) || !__STRICT_ANSI__ */

#ifndef __STRICT_ANSI__

int	strerror_r(int _errnum, char *_strerrbuf, size_t _buflen);
char *	strtok_r(char *  _s1, const char *  _s2, char ** _s3);

#ifndef _POSIX_SOURCE

#include <sys/movedata.h>

int	bcmp(const void *_ptr1, const void *_ptr2, size_t _length);
void 	bcopy(const void *_a, void *_b, size_t _len);
void 	bzero(void *ptr, size_t _len);
int	ffs(int _mask);
char *  index(const char *_string, int _c);
void *	memccpy(void *_to, const void *_from, int _c, size_t _n);
int	memicmp(const void *_s1, const void *_s2, size_t _n);
void *	memmem(const void *l, size_t l_len, const void *s, size_t s_len);
char *  rindex(const char *_string, int _c);
char *	stpcpy(char *_dest, const char *_src);
char *	stpncpy(char *_dest, const char *_src, size_t _n);
char *	strdup(const char *_s);
char *	strndup(const char *_s, size_t _n);
size_t	strlcat(char *_dest, const char *_src, size_t _size);
size_t	strlcpy(char *_dest, const char *_src, size_t _size);
char *	strlwr(char *_s);
int	strcasecmp(const char *_s1, const char *_s2);
int	stricmp(const char *_s1, const char *_s2);
int	strncasecmp(const char *_s1, const char *_s2, size_t _n);
int	strnicmp(const char *_s1, const char *_s2, size_t _n);
size_t	strnlen(const char *_s, size_t _n);
char *	strsep(char **_stringp, const char *_delim);
char *	strupr(char *_s);

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_string_h_ */
