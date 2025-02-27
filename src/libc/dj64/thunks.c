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
 * NOTE: this file is a port of thunks.cc file from fdpp project.
 * In fdpp project it is distributed under the terms of GNU GPLv3+
 * and is copyrighted (C) 2018-2023  Stas Sergeev (stsp).
 * As a sole author of the aforementioned dosobj.cc, I donate the
 * code to dj64dev project, allowing to re-license it under the terms
 * of GNU LGPLv3+.
 *
 * --stsp
 */

#include <setjmp.h>
#include <assert.h>
#include <djdev64/dj64init.h>
#include <dj64/thunks_p.h>
#include <dj64/util.h>
#include <libc/djthunks.h>
#include <libc/djctx.h>
#include <libc/internal.h>
#include <dpmi.h>
#include <go64.h>
#include <sys/nearptr.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <crt0.h>
#include "thunks_a.h"
#include "thunks_c.h"
#include "plt.h"
#include "dosobj.h"

int __crt0_startup_flags;

struct udisp {
    dj64dispatch_t *disp;
    const struct elf_ops *eops;
    struct athunks *at;
    struct athunks *pt;
    struct athunks core_at;
    struct athunks core_pt;
    unsigned cs;
    main_t *main;
    int full_init;
    dpmi_regs s_regs;
    int recur_cnt;
    int objcnt;
    jmp_buf *noret_jmp;
#define MAX_OBJS 50
#define MAX_RECUR 10
    uint32_t objs[MAX_RECUR][MAX_OBJS];
};
#define MAX_HANDLES 10
static struct udisp udisps[MAX_HANDLES];
static const struct dj64_api *dj64api;
static int dj64api_ver;
static dj64dispatch_t *disp_fn;
static struct athunks *u_athunks;
static struct athunks *u_pthunks;
static int *u_handle_p;

struct ctx_hooks {
    void (*init)(int);
    void (*deinit)(void);
    void (*save)(void);
    void (*restore)(int);
};

#define MAX_CTX_HOOKS 100
static struct ctx_hooks chooks[MAX_CTX_HOOKS];
static int num_chooks;

static void do_rm_dosobj(struct udisp *u, uint32_t fa);

void djloudvprintf(const char *format, va_list vl)
{
    va_list cpy;

    va_copy(cpy, vl);
    dj64api->print(DJ64_PRINT_LOG, format, cpy);
    va_end(cpy);
    dj64api->print(DJ64_PRINT_TERMINAL, format, vl);
}

void djloudprintf(const char *format, ...)
{
    va_list vl;

    va_start(vl, format);
    djloudvprintf(format, vl);
    va_end(vl);
}

void djlogprintf(const char *format, ...)
{
    va_list vl;

    va_start(vl, format);
    dj64api->print(DJ64_PRINT_LOG, format, vl);
    va_end(vl);
}

uint8_t *djaddr2ptr(uint32_t addr)
{
    return dj64api->addr2ptr(addr);
}

uint8_t *djaddr2ptr2(uint32_t addr, uint32_t len)
{
    return dj64api->addr2ptr2(addr, len);
}

uint32_t djptr2addr(const uint8_t *ptr)
{
    return dj64api->ptr2addr(ptr);
}

static int _dj64_call(struct udisp *u, int libid, int fn, dpmi_regs *regs,
    uint8_t *sp, unsigned esi, dj64dispatch_t *disp, int handle)
{
    int len;
    UDWORD res;
    int rc;
    jmp_buf noret;

    u->s_regs = *regs;
    if ((rc = setjmp(noret))) {
        int i;

        for (i = 0; i < num_chooks; i++)
            chooks[i].restore(handle);
        /* gc lost objects, esp in ABORT case */
        for (i = 0; i < MAX_OBJS; i++) {
            if (u->objs[u->recur_cnt - 1][i])
                do_rm_dosobj(u, u->objs[u->recur_cnt - 1][i]);
        }
        return (rc == ASM_NORET ? DJ64_RET_NORET : DJ64_RET_ABORT);
    }
    u->noret_jmp = &noret;
    res = (libid ? disp : dj64_thunk_call)(fn, sp, &len);
    *regs = u->s_regs;
    switch (len) {
    case 0:
        break;
    case 1:
    case 2:
    case 4:
        regs->eax = res;
        break;
    default:
//        _fail();
        break;
    }
    return DJ64_RET_OK;
}

static int dj64_call(int handle, int libid, int fn, unsigned esi, uint8_t *sp)
{
    int i;
    int ret;
    int last_objcnt;
    struct udisp *u;
    jmp_buf *saved_noret;
    dpmi_regs *regs = (dpmi_regs *)sp;

    sp += sizeof(*regs) + 8;  // skip regs, ebp, eip to get stack args
    assert(handle < MAX_HANDLES);
    u = &udisps[handle];
    assert(u->recur_cnt < MAX_RECUR);
    u->recur_cnt++;
    for (i = 0; i < num_chooks; i++)
        chooks[i].restore(handle);
    saved_noret = u->noret_jmp;
    last_objcnt = u->objcnt;
    ret = _dj64_call(u, libid, fn, regs, sp, esi, u->disp, handle);
    assert(u->objcnt == last_objcnt);  // make sure no leaks, esp on NORETURN
    u->noret_jmp = saved_noret;
    if (ret == DJ64_RET_OK) {
        for (i = 0; i < num_chooks; i++)
            chooks[i].save();
    }
    u->recur_cnt--;
    return ret;
}

static int process_athunks(struct athunks *at, uint32_t mem_base,
	const struct elf_ops *eops, void *eh)
{
    int i, ret = 0;

    for (i = 0; i < at->num; i++) {
        const struct athunk *t = &at->at[i];
        uint32_t off = eops->getsym(eh, t->name);
        if (off) {
            at->tab[i] = mem_base + off;
        } else {
            djloudprintf("symbol %s not resolved\n", t->name);
            ret = -1;
            break;
        }
    }
    return ret;
}

static int process_pthunks(struct athunks *pt,
	const struct elf_ops *eops, void *eh)
{
    int i, ret = 0;

    for (i = 0; i < pt->num; i++) {
        const struct athunk *t = &pt->at[i];
        pt->tab[i] = eops->getsym(eh, t->name);
        if (!pt->tab[i]) {
            djloudprintf("symbol %s not resolved\n", t->name);
            ret = -1;
            break;
        }
    }
    return ret;
}

static ASMh(int, _crt0_startup_flags)

static void do_early_init(int handle)
{
    *____crt0_startup_flags(handle) = __crt0_startup_flags;
}

static int dj64_ctrl(int handle, int libid, int fn, unsigned esi, uint8_t *sp)
{
    dpmi_regs *regs = (dpmi_regs *)sp;
    int ver = libid >> 8;
    assert(handle < MAX_HANDLES);
    if (ver != DL_API_VER) {
        djloudprintf("dj64: API version mismatch, got %i want %i\n",
                     ver, DL_API_VER);
        if (ver == 0)  // this doesn't even handle errors, so terminate
            dj64api->exit(1);
        return -1;
    }
    switch (fn) {
    case DL_SET_SYMTAB: {
        __label__ err;
        struct udisp *u = &udisps[handle];
        uint32_t flags = regs->eax;
        int have_core = (flags & 0x4000);
        uint32_t addr = regs->ebx;
        uint32_t size = regs->ecx;
        uint32_t mem_base = regs->edx;
        int32_t cpl_fd = (regs->esi == (uint32_t)-1 ? -1 :
                dj64api->uget(regs->esi));
        void *eh = NULL;
        int ret;

        u->cs = esi;
        djlogprintf("addr 0x%x mem_base 0x%x\n", addr, mem_base);
        if (addr) {
            char *elf = (char *)djaddr2ptr(addr);
            djlogprintf("data %p(%s)\n", elf, elf);
            eh = u->eops->open(elf, size);
            if (!eh)
                return -1;
            if (have_core) {
                ret = process_athunks(&u->core_at, mem_base, u->eops, eh);
                if (ret)
                    goto err;
                ret = process_pthunks(&u->core_pt, u->eops, eh);
                if (ret)
                    goto err;
            }
            if (u->at) {
                ret = process_athunks(u->at, mem_base, u->eops, eh);
                if (ret)
                    goto err;
            }
            if (u->pt) {
                ret = process_pthunks(u->pt, u->eops, eh);
               if (ret)
                    goto err;
            }
            u->eops->close(eh);
        } else if (have_core) {
            return -1;
        }
        if (!have_core) {
            if (cpl_fd == -1)
                return -1;
            eh = u->eops->open_dyn(cpl_fd);
            if (!eh)
                return -1;
            ret = process_athunks(&u->core_at, mem_base, u->eops, eh);
            if (ret)
                goto err;
            ret = process_pthunks(&u->core_pt, u->eops, eh);
            if (ret)
                goto err;
            u->eops->close(eh);
        }

        do_early_init(handle);
        return 0;
    err:
        u->eops->close(eh);
        break;
    }

    case DL_ELFLOAD: {
        __label__ err;
        struct udisp *u = &udisps[handle];
        uint32_t addr = regs->ebx;
        uint32_t size = regs->ecx;
        uint32_t mem_base = regs->edx;
        void *eh = NULL;
        int ret;
        uint32_t esize, entry;
        char *elf = dj64api->elfparse64(0, addr, size, mem_base, &esize,
                &entry);
        if (!elf)
            return -1;
        eh = u->eops->open(elf, esize);
        if (!eh)
            goto err2;
        if (u->at) {
            ret = process_athunks(u->at, mem_base, u->eops, eh);
            if (ret)
                goto err;
        }
        if (u->pt) {
            ret = process_pthunks(u->pt, u->eops, eh);
            if (ret)
                goto err;
        }
        u->eops->close(eh);
        dj64api->free(elf);
        regs->eax = entry;
        return 0;
    err:
        u->eops->close(eh);
    err2:
        dj64api->free(elf);
        break;
    }
    }
    return -1;
}

static dj64cdispatch_t *dops[] = { dj64_call, dj64_ctrl };

dj64cdispatch_t **DJ64_INIT_FN(int handle, const struct elf_ops *ops,
        void *main, int full_init)
{
    int i;
    struct udisp *u;

    assert(handle < MAX_HANDLES);
    u = &udisps[handle];
    u->disp = disp_fn;
    disp_fn = NULL;
    u->eops = ops;
    u->main = main;
    u->full_init = full_init;

    u->at = u_athunks;
    u_athunks = NULL;

    u->pt = u_pthunks;
    if (u_handle_p)
        *u_handle_p = handle;
    u_pthunks = NULL;
    u_handle_p = NULL;

    u->core_at = asm_thunks;
    u->core_at.tab = dj64api->malloc(sizeof(asm_thunks.tab[0]) * asm_thunks.num);
    u->core_pt = pthunks;
    u->core_pt.tab = dj64api->malloc(sizeof(pthunks.tab[0]) * pthunks.num);

    for (i = 0; i < num_chooks; i++)
        chooks[i].init(handle);

    return dops;
}

void DJ64_DONE_FN(int handle)
{
    int i;
    struct udisp *u;

    for (i = 0; i < num_chooks; i++)
        chooks[i].deinit();
    assert(handle < MAX_HANDLES);
    u = &udisps[handle];
    dj64api->free(u->core_at.tab);
    dj64api->free(u->core_pt.tab);
}

int DJ64_INIT_ONCE_FN(const struct dj64_api *api, int api_ver)
{
    int ret = 0;
    if (api_ver < DJ64_API_MIN_VER)
        return -1;
    if (!dj64api)
        ret++;
    dj64api = api;
    dj64api_ver = api_ver;
    return ret;
}

static uint64_t do_asm_call(struct udisp *u, struct athunks *pt,
        unsigned cs, int num, uint8_t *sp, uint8_t len, int flags)
{
    int rc;
    dpmi_paddr pma;
    assert(num < pt->num);
    pma.selector = cs;
    pma.offset32 = pt->tab[num];
    if (flags & _TFLG_NORET) {
        djlogprintf("NORET call %s: 0x%x:0x%x\n", pt->at[num].name,
            pma.selector, pma.offset32);
        dj64api->asm_noret(&u->s_regs, pma, sp, len);
        longjmp(*u->noret_jmp, ASM_NORET);
    }
    djlogprintf("asm call %s: 0x%x:0x%x\n", pt->at[num].name,
            pma.selector, pma.offset32);
    rc = dj64api->asm_call(&u->s_regs, pma, sp, len);
    djlogprintf("asm call %s returned %i:0x%x\n", pt->at[num].name,
            rc, u->s_regs.eax);
    switch (rc) {
    case ASM_CALL_OK:
        break;
    case ASM_CALL_ABORT:
        djlogprintf("reboot jump, %i\n", u->recur_cnt);
        longjmp(*u->noret_jmp, ASM_ABORT);
        break;
    }
    return u->s_regs.eax | ((uint64_t)u->s_regs.edx << 32);
}

uint64_t dj64_asm_call(int num, uint8_t *sp, uint8_t len, int flags)
{
    int i;
    int ret;
    struct udisp *u;
    int handle = dj64api->get_handle();
    assert(handle < MAX_HANDLES);
    for (i = 0; i < num_chooks; i++)
        chooks[i].save();
    u = &udisps[handle];
    ret = do_asm_call(u, &u->core_pt, u->cs, num, sp, len, flags);
    /* asm call can recursively invoke dj64, so restore context here */
    for (i = 0; i < num_chooks; i++)
        chooks[i].restore(handle);
    return ret;
}

uint64_t dj64_asm_call_u(int handle, int num, uint8_t *sp, uint8_t len,
        int flags)
{
    int i;
    int ret;
    struct udisp *u;

    assert(handle < MAX_HANDLES);
    u = &udisps[handle];
    if (!u->pt) {
        djloudprintf("no user thunks\n");
        return ASM_CALL_ABORT;
    }
    for (i = 0; i < num_chooks; i++)
        chooks[i].save();
    ret = do_asm_call(u, u->pt, u->cs, num, sp, len, flags);
    /* asm call can recursively invoke dj64, so restore context here */
    for (i = 0; i < num_chooks; i++)
        chooks[i].restore(handle);
    return ret;
}

uint8_t *dj64_clean_stk(size_t len)
{
    return dj64api->inc_esp(len);
}

static uint32_t *find_oh(uint32_t addr, uint32_t objs[MAX_RECUR][MAX_OBJS],
        int recur_cnt)
{
    int i;

    for (i = 0; i < MAX_OBJS; i++) {
        if (objs[recur_cnt - 1][i] == addr)
            return &objs[recur_cnt - 1][i];
    }
    return NULL;  // should not happen
}

static uint32_t do_obj_init(const void *data, uint16_t len, int is_out)
{
    uint32_t ret;
    int handle;
    struct udisp *u;

    if (dj64api->is_dos_ptr(data))
        return PTR_DATA(data);
    handle = dj64api->get_handle();
    assert(handle < MAX_HANDLES);
    u = &udisps[handle];
    ret = mk_dosobj(len);
    if (!is_out)
        pr_dosobj(ret, data, len);
    u->objcnt++;
    *find_oh(0, u->objs, u->recur_cnt) = ret;
    return ret;
}

uint32_t dj64_obj_init(const void *data, uint16_t len)
{
    return do_obj_init(data, len, 0);
}

uint32_t dj64_obj_oinit(const void *data, uint16_t len)
{
    return do_obj_init(data, len, 1);
}

void dj64_obj_done(void *data, uint32_t fa, uint16_t len)
{
    int handle;
    struct udisp *u;

    if (dj64api->is_dos_ptr(data))
        return;
    handle = dj64api->get_handle();
    assert(handle < MAX_HANDLES);
    u = &udisps[handle];
    cp_dosobj(data, fa, len);
    rm_dosobj(fa);
    *find_oh(fa, u->objs, u->recur_cnt) = 0;
    u->objcnt--;
}

static void do_rm_dosobj(struct udisp *u, uint32_t fa)
{
    rm_dosobj(fa);
    *find_oh(fa, u->objs, u->recur_cnt) = 0;
    u->objcnt--;
}

void dj64_rm_dosobj(const void *data, uint32_t fa)
{
    int handle;
    struct udisp *u;

    if (dj64api->is_dos_ptr(data))
        return;
    handle = dj64api->get_handle();
    assert(handle < MAX_HANDLES);
    u = &udisps[handle];
    do_rm_dosobj(u, fa);
}

void register_dispatch_fn(dj64dispatch_t *fn)
{
    assert(!disp_fn);
    disp_fn = fn;
}

void register_athunks(struct athunks *at)
{
    assert(!u_athunks);
    u_athunks = at;
}

void register_pthunks(struct athunks *pt, int *handle_p)
{
    assert(!u_pthunks);
    u_pthunks = pt;
    u_handle_p = handle_p;
}

void crt1_startup(int handle)
{
    struct udisp *u;

    assert(handle < MAX_HANDLES);
    __djgpp_nearptr_enable();  // speeds up things considerably
    dosobj_init(dosobj_page, 4096);

    u = &udisps[handle];
    __crt1_startup(u->main);
}

static uint32_t do_thunk_get(const struct athunks *at, const char *name)
{
    int i;

    for (i = 0; i < at->num; i++) {
        if (strcmp(at->at[i].name, name) == 0)
            return at->tab[i];
    }
    return (uint32_t)-1;
}

uint32_t djthunk_get_h(int handle, const char *name)
{
    struct udisp *u;
    uint32_t ret = (uint32_t)-1;

    assert(handle < MAX_HANDLES);
    u = &udisps[handle];
    if (u->at)
        ret = do_thunk_get(u->at, name);
    if (ret == (uint32_t)-1)
        ret = do_thunk_get(&u->core_at, name);
    assert(ret != (uint32_t)-1);
    return ret;
}

uint32_t djthunk_get(const char *name)
{
    return djthunk_get_h(dj64api->get_handle(), name);
}

void djregister_ctx_hooks(void (*init)(int), void (*deinit)(void),
        void (*save)(void), void (*restore)(int))
{
    struct ctx_hooks *c;

    assert(num_chooks < MAX_CTX_HOOKS);
    c = &chooks[num_chooks++];
    c->init = init;
    c->deinit = deinit;
    c->save = save;
    c->restore = restore;
}

void *djsbrk(int increment)
{
    assert(increment > 0);
    return dj64api->malloc(increment);
}

int elfload(int num)
{
    if (dj64api_ver < 15)
        return -1;
    return dj64api->elfload(num);
}
