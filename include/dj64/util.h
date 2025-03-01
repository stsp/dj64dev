/*
 *  dj64 - 64bit djgpp-compatible tool-chain
 *  Copyright (C) 2021-2024  @stsp
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

#ifndef DJ64UTIL_H
#define DJ64UTIL_H

#include <stdint.h>
#include <stdarg.h>

#define DJ64_DYNAPI_VER 2

#define PRINTF(n) __attribute__((format(printf, n, n + 1)))
void djloudprintf(const char *format, ...) PRINTF(1);
void djlogprintf(const char *format, ...) PRINTF(1);
void djloudvprintf(const char *format, va_list vl);

typedef uint32_t (dj64dispatch_t)(int fn, uint8_t *sp, int *r_len);
void register_dispatch_fn(dj64dispatch_t *fn, int ver);

struct athunk {
    const char *name;
};
struct athunks {
    const struct athunk *at;
    int num;
    uint32_t *tab;
};
void register_athunks(struct athunks *at, int ver);
void register_pthunks(struct athunks *pt, int *handle_p, int *libid_p,
        int ver);

#define _countof(array) (sizeof(array) / sizeof(array[0]))

#define UBYTE uint8_t
#define UDWORD uint32_t
#define DWORD int32_t
#define UWORD uint16_t
#define WORD int16_t
#define VOID void

enum { ASM_OK, ASM_NORET, ASM_ABORT, PING_ABORT };

#endif
