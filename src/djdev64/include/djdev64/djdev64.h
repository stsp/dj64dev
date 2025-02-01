/*
 *  dj64 - 64bit djgpp-compatible tool-chain
 *  Copyright (C) 2021-2024  @stsp
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DJDEV64_H
#define DJDEV64_H

#include "djdev64/dj64init.h"

/* first 16 bits are for internal use */
#define DJ64F_DLMOPEN (1 << 16)

int djdev64_open(const char *path, const struct dj64_api *api, int api_ver,
        unsigned flags);
int djdev64_call(int handle, int libid, int fn, unsigned esi,
        unsigned char *sp);
int djdev64_ctrl(int handle, int libid, int fn, unsigned esi,
        unsigned char *sp);
void djdev64_close(int handle);

int djdev64_exec(const char *path, unsigned flags, int argc, char **argv);
int djdev64_add_load_path(const char *path);
int djdev64_load(int num, unsigned flags);

#endif
