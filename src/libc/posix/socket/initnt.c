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
 * initnt.c
 * Written by Richard Dawe
 *
 * Initialise all networking services.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lsck/copyrite.h>
#include <lsck/lsck.h>
#include <lsck/ini.h>
#include <lsck/if.h>

extern LSCK_IF *__wsock_init (void);
extern LSCK_IF *__csock_init (void);
extern LSCK_IF *__unix_init (void);

/* Copyright information */
static char lsck_version[80] = ""; /* Line of text at maximum */

static char lsck_copyright[] =
"Copyright (C) 1997, 1998 by Indrek Mandre, 1997-2000 by Richard Dawe";

/* Private variables */
static int __lsck_inited = 0;

LSCK_IF *__lsck_interface[LSCK_MAX_IF + 1];

typedef LSCK_IF * (*if_init_func_t) (void);

typedef struct {
	int            number;
	if_init_func_t init;
	const char    *init_name;
} if_init_t;

static if_init_t if_init_table[LSCK_MAX_IF + 1] = {
	/* Winsock support - WSOCK.VXD */
//	{ LSCK_IF_WSOCK, __wsock_init, "wsock" },
	/* Coda filesystem socket helper - SOCK.VXD */
	{ LSCK_IF_CSOCK, __csock_init, "csock" },
	/* Unix domain sockets over mailslots */
//	{ LSCK_IF_UNIX, __unix_init, "unix" },
	{ -1, NULL, NULL }
};

extern LSCK_SOCKET *__lsck_socket[__LSCK_NUM_SOCKET];

/* Debugging support */
static int __lsck_debug_level = 0;	/* None by default */

void __inline__ __lsck_debug_setlevel (int level)
{
	__lsck_debug_level = level;
}

int __inline__  __lsck_debug_getlevel (void) {
	return(__lsck_debug_level);
}

void __inline__ __lsck_debug_enable (void)
{
    __lsck_debug_setlevel (LSCK_DEBUG_ON);
}

void __inline__ __lsck_debug_disable (void)
{
    __lsck_debug_setlevel (LSCK_DEBUG_OFF);
}

int __inline__ __lsck_debug_enabled (void)
{
    return ((__lsck_debug_getlevel () != LSCK_DEBUG_OFF) ? 1 : 0);
}

/* ---------------------
 * - init_version_info -
 * --------------------- */

/*
 * Set up textual versions of the version info in include/lsck/copyrite.h,
 * for use in user programs.
 */

static int init_version_info (void)
{
	static int version_info_inited = 0;

	if (version_info_inited) return(version_info_inited);

#ifdef LSCK_VERSION_RELEASED
/* Released */
#if defined(LSCK_VERSION_ALPHA) && (LSCK_VERSION_ALPHA > 0)
	sprintf (lsck_version, "libsocket version %i.%i.%i alpha %i",
		 LSCK_VERSION_MAJOR, LSCK_VERSION_MINOR, LSCK_VERSION_SUBMINOR,
		 LSCK_VERSION_ALPHA);
#elif defined(LSCK_VERSION_BETA) && (LSCK_VERSION_BETA > 0)
	sprintf (lsck_version, "libsocket version %i.%i.%i beta %i",
		 LSCK_VERSION_MAJOR, LSCK_VERSION_MINOR, LSCK_VERSION_SUBMINOR,
		 LSCK_VERSION_BETA);
#elif defined(LSCK_VERSION_PRE) && (LSCK_VERSION_PRE > 0)
	sprintf (lsck_version, "libsocket version %i.%i.%i pre %i",
		 LSCK_VERSION_MAJOR, LSCK_VERSION_MINOR, LSCK_VERSION_SUBMINOR,
		 LSCK_VERSION_PRE);
#else
	sprintf (lsck_version, "libsocket version %i.%i.%i",
		 LSCK_VERSION_MAJOR, LSCK_VERSION_MINOR,
		 LSCK_VERSION_SUBMINOR);
#endif
#else /* !LSCK_VERSION_RELEASED */
/* Pre-releases */
#if defined(LSCK_VERSION_ALPHA) && (LSCK_VERSION_ALPHA > 0)
	sprintf (lsck_version, "libsocket version %i.%i.%i alpha %i "
		 "(pre-release %s)",
		 LSCK_VERSION_MAJOR, LSCK_VERSION_MINOR, LSCK_VERSION_SUBMINOR,
		 LSCK_VERSION_ALPHA, LSCK_VERSION_DATE);
#elif defined(LSCK_VERSION_BETA) && (LSCK_VERSION_BETA > 0)
	sprintf (lsck_version, "libsocket version %i.%i.%i beta %i "
		 "(pre-release %s)",
		 LSCK_VERSION_MAJOR, LSCK_VERSION_MINOR, LSCK_VERSION_SUBMINOR,
		 LSCK_VERSION_BETA, LSCK_VERSION_DATE);
#elif defined(LSCK_VERSION_PRE) && (LSCK_VERSION_PRE > 0)
	sprintf (lsck_version, "libsocket version %i.%i.%i pre %i "
		 "(pre-release %s)",
		 LSCK_VERSION_MAJOR, LSCK_VERSION_MINOR, LSCK_VERSION_SUBMINOR,
		 LSCK_VERSION_PRE, LSCK_VERSION_DATE);
#else
	sprintf (lsck_version, "libsocket version %i.%i.%i"
		 "(pre-release %s)",
		 LSCK_VERSION_MAJOR, LSCK_VERSION_MINOR,
		 LSCK_VERSION_SUBMINOR, LSCK_VERSION_DATE);
#endif
#endif /* LSCK_VERSION_RELEASED */

	version_info_inited = 1;
	return(version_info_inited);
}

/* ------------------------
 * - __lsck_get_copyright -
 * ------------------------ */

char *__lsck_get_copyright (void)
{
	if (!init_version_info()) return(NULL);
	return(lsck_copyright);
}

/* ----------------------
 * - __lsck_get_version -
 * ---------------------- */

char *__lsck_get_version (void)
{
	if (!init_version_info()) return(NULL);
	return(lsck_version);
}

/* ---------------
 * - __lsck_init -
 * --------------- */

int __lsck_init (void)
{
	LSCK_IF *interface = NULL;
	char *cfg = NULL;
	char debug_buf[16];
	char num_buf[33];
	int i, j;

	if (__lsck_inited != 0) return (__lsck_inited);

	/* --- Initialisation --- */

	/* Set up the version info */
	if (!init_version_info()) return(0);

	/* Check for debugging options */
#ifdef DEBUG_LIBSOCKET
	printf ("libsocket: Use of the define DEBUG_LIBSOCKET is deprecated. "
		"Please use the\n"
		"libsocket: configuration file option 'debug' in the 'main' "
		"section.\n");
	__lsck_debug_setlevel (LSCK_DEBUG_VERBOSE);
#endif

	/* Debug config option */
	cfg = __lsck_config_getfile();

	if (cfg == NULL)
		strcpy(debug_buf, "no");
	else
		GetPrivateProfileString ("main", "debug", "false",
					 debug_buf, sizeof (debug_buf), cfg);

	if ((stricmp (debug_buf, "false") == 0)
	    || (stricmp (debug_buf, "no") == 0)
	    || (strcmp (debug_buf, "0") == 0)) {
		__lsck_debug_disable ();
	} else {
		/* Check for verbose option */
		itoa (LSCK_DEBUG_VERBOSE, num_buf, 10);
		if ((stricmp (debug_buf, "verbose") == 0)
		    || (strcmp (debug_buf, num_buf) == 0)) {
			__lsck_debug_setlevel (LSCK_DEBUG_VERBOSE);
			printf ("libsocket: Reporting verbose debugging "
				"messages\n");
		} else {
			__lsck_debug_setlevel (LSCK_DEBUG_NORMAL);
		}
	}

	/* Clear all descriptors */
	for (i = 0; i < __LSCK_NUM_SOCKET; i++) {
		__lsck_socket[i] = NULL;
	}

	/* Clear all interface info */
	for (i = 0; i < (LSCK_MAX_IF + 1); i++) {
		__lsck_interface[i] = NULL;
	}

	/* --- Initialise all interfaces --- */
	for (i = 0, j = 0; if_init_table[i].number >= 0; i++) {
		interface = if_init_table[i].init();

		if (interface != NULL) {
			/* Debug info */
			if (__lsck_debug_enabled ()) {
				/* Module pseudo-name */
				printf("%s: Initialised successfully\n",
				       if_init_table[i].init_name);

				/* Module description */
				printf("%s: %s\n",
				       if_init_table[i].init_name,
				       interface->name);
			}

			__lsck_interface[j] = interface;
			j++;
		}
	}

	/* Fail if there are no interfaces available */
	if (__lsck_interface[0] == NULL) {
		__lsck_inited = 1;
		__lsck_uninit ();
		return (0);
	}

	/* Set up atexit() handler */
	atexit (__lsck_uninit);

	/* Done */
	__lsck_inited = 1;
	return (__lsck_inited);
}

/* -----------------
 * - __lsck_uninit -
 * ----------------- */

void __lsck_uninit (void)
{
	int i;

	if (!__lsck_inited) return;

	/* Pre-uninitialisation calls */
	for (i = 0; __lsck_interface[i] != NULL; i++) {
		if (__lsck_interface[i]->preuninit != NULL)
			__lsck_interface[i]->preuninit ();
	}

	/* Free all descriptors after closing the sockets */
	for (i = 0; i < __LSCK_NUM_SOCKET; i++) {
		if (__lsck_socket[i] != NULL) {
			close (__lsck_socket[i]->fd);
			free (__lsck_socket[i]);
		}
	}

	/* Uninitialise all interfaces */
	for (i = 0; __lsck_interface[i] != NULL; i++) {
		if (__lsck_interface[i]->uninit != NULL)
			__lsck_interface[i]->uninit ();
	}
}
