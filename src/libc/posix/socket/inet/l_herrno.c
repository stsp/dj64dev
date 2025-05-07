/*
 * l_herrno.c
 * Copyright unknown - presumably this is part of the Linux networking code.
 *
 * This file is part of libsocket.
 * libsocket Copyright 1997, 1998 by Indrek Mandre
 * libsocket Copyright 1997-2000 by Richard Dawe
 */

#include <sys/socket.h>

#undef h_errno

/* The one in libpthread will override __h_errno_location () */
int *__normal_h_errno_location ( void );

/* Richard Dawe (libsocket): The commented code below was used. Since the
 * aliasing isn't very useful for DJGPP programs and stops me cross-compiling,
 * just point h_errno and __h_errno_location_location at the relevant
 * function. */

/* Keep gcc 2.95.x happy. */
/*extern int h_errno __attribute__ ((weak, alias("_h_errno")));*/
/*#pragma weak h_errno = _h_errno*/

/*int _h_errno = 0;*/
int h_errno = 0;

/*#pragma weak __h_errno_location = __normal_h_errno_location*/

/*int *__h_errno_location (void)
  __attribute__ ((weak, alias("__normal_h_errno_location__")));*/

typedef int * (*h_errno_loc_func) (void);
h_errno_loc_func __h_errno_location = __normal_h_errno_location;

int *
__normal_h_errno_location ( void )
{
  return &h_errno;
}
