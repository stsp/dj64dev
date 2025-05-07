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
 * windows.c - General functions for libsocket, when it's running under
 * Windows.
 */

#include <string.h>
#include <dpmi.h>
#include <dos.h>

#include <lsck/windows.h>

/* -------------------------
 * - __get_windows_version -
 * ------------------------- */

int __get_windows_version (void)
{
	__dpmi_regs r;
	int win_version;

	/* Get the windows version with INT 0x2F sub-function 0x160A,
	 * Windows 3.1+ installation check. This version check isn't
	 * amazing, but is probably enough because if the version isn't right
	 * then the VxDs will fail to load anyway. */
	bzero((void *) &r, sizeof(r));
	r.x.ax = 0x160A;
	__dpmi_int(0x2F, &r);

	if (r.h.bh == 3) {
		win_version = WIN_311;
	} else if (r.h.bh > 3) {
		/* TODO: Distinguish between '95, '98? */
		/* Windows '95 or '98 */
		win_version = WIN_95;
	} else {
		win_version = WIN_NONE;
	}

	/* If win_version is WIN_NONE, then we need to use the DOS version to
	 * determine if this is Windows NT. Thanks to Allegro (Shawn Hargreaves
	 * et al) for this method. It's also in Ralph Brown's Interrupt list,
	 * INT 0x21, AX = 0x3306. */
	if (win_version == WIN_NONE) {
		if (_get_dos_version(1) == 0x532) {
			win_version = WIN_NT;
		}
	}

	return(win_version);
}
