/*
 *  dj64 - 64bit djgpp-compatible tool-chain
 *  Copyright (C) 2021-2026  @stsp
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

#include <djdev64/dj64init.h>
#include <libc/internal.h>
#include <stdint.h>
#include <stdlib.h>

static uint8_t *addr2ptr(uint32_t addr)
{
    return (uint8_t *)addr;
}

static uint8_t *addr2ptr2(uint32_t addr, uint32_t len)
{
    return (uint8_t *)addr;
}

static uint32_t ptr2addr(const uint8_t *ptr)
{
    return (uintptr_t)ptr;
}

static void print(int prio, const char *format, va_list ap)
{
}

static int asm_call(dpmi_regs *regs, dpmi_paddr pma, uint8_t *sp, uint8_t len)
{
    return 0;
}

static void asm_noret(dpmi_regs *regs, dpmi_paddr pma, uint8_t *sp,
            uint8_t len)
{
}

static uint8_t *inc_esp(uint32_t len)
{
    return NULL;
}

static int is_dos_ptr(const uint8_t *ptr)
{
    return 1;
}

static int get_handle(void)
{
    return 0;
}

static struct dj64_api dj32 = {
    addr2ptr,
    addr2ptr2,
    ptr2addr,
    print,
    asm_call,
    asm_noret,
    inc_esp,
    is_dos_ptr,
    get_handle,
    exit,
    malloc,
    free,
    NULL,  // elfload
};

int dj32_init(void)
{
    int ret = DJ64_INIT_ONCE_FN(&dj32, DJ64_API_VER);
    DJ64_INIT_FN(0, NULL, NULL, 0);
    return ret;
}
