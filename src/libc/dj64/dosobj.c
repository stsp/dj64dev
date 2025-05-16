/*
 *  dj64 - 64bit djgpp-compatible tool-chain
 *  Copyright (C) 2021-2024  @stsp
 *
 *  FDPP - freedos port to modern C++
 *  Copyright (C) 2018  Stas Sergeev (stsp)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * NOTE: this file is a port of dosobj.cc file from fdpp project.
 * In fdpp project it is distributed under the terms of GNU GPLv3+
 * and is copyrighted (C) 2018  Stas Sergeev (stsp).
 * As a sole author of the aforementioned dosobj.cc, I donate the
 * code to dj64dev project, allowing to re-license it under the terms
 * of GNU LGPLv3+.
 *
 * --stsp
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <dpmi.h>
#include <dj64/util.h>
#include <libc/djthunks.h>
#include "dosobj.h"

void dosobj_init(void)
{
}

void dosobj_free(void)
{
}

uint32_t mk_dosobj(uint32_t len)
{
    return malloc32(len);
}

void pr_dosobj(uint32_t fa, const void *data, uint32_t len)
{
    void *ptr = DATA_PTR(fa);

    memcpy(ptr, data, len);
}

void cp_dosobj(void *data, uint32_t fa, uint32_t len)
{
    void *ptr = DATA_PTR(fa);

    memcpy(data, ptr, len);
}

void rm_dosobj(uint32_t fa)
{
    free32(fa);
}
