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

#include <stddef.h>
#include <stdint.h>
#include "util.h"

uint64_t dj64_asm_call(int num, uint8_t *sp, uint8_t len,
    int flags);
uint64_t dj64_asm_call_u(int handle, int libid, int num, uint8_t *sp,
    uint8_t len, int flags);
NORETURN
void dj64_asm_noret(int num, uint8_t *sp, uint8_t len,
    int flags);
NORETURN
void dj64_asm_noret_u(int handle, int libid, int num, uint8_t *sp,
    uint8_t len, int flags);
uint8_t *dj64_clean_stk(size_t len);
uint32_t dj64_obj_init(const void *data, uint32_t len);
uint32_t dj64_obj_oinit(const void *data, uint32_t len);
void dj64_obj_done(void *data, uint32_t fa, uint32_t len);
void dj64_rm_dosobj(const void *data, uint32_t fa);

#define _TFLG_NONE 0
#define _TFLG_FAR 1
#define _TFLG_NORET 2
#define _TFLG_INIT 4

#ifdef _IN_DJ64
extern struct athunks pthunks;
#endif
