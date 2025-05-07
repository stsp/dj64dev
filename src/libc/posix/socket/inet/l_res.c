/*
 * l_res.c
 * Copyright unknown - presumably this is part of the Linux networking code.
 *
 * This file is part of libsocket.
 * libsocket Copyright 1997, 1998 by Indrek Mandre
 * libsocket Copyright 1997-2000 by Richard Dawe
 */

#include <sys/param.h>
#include <ctype.h>

#include <resolv.h>

/* Richard Dawe (libsocket): Let's keep gcc 2.95.x happy. */
/*struct __res_state *__res_status_location (void)
  __attribute__ ((weak, alias("__normal_res_status_location")));*/

/* The one in libpthread will override __res_status_location () */
struct __res_state *__normal__res_status_location ( void );

/* Richard Dawe (libsocket): The commented code below was used. Since the
 * aliasing isn't very useful for DJGPP programs and stops me cross-compiling,
 * just point __res_status_location at the relevant function. */

/*#pragma weak __res_status_location = __normal__res_status_location*/

typedef struct __res_state * (*res_state_loc_func) (void);
res_state_loc_func __res_status_location = __normal__res_status_location;

struct __res_state *
__normal__res_status_location ( void )
{
  return &_res;
}
