/*
 *  go32-compatible COFF, PE32 and ELF loader stub.
 *  Copyright (C) 2022,  stsp <stsp@users.sourceforge.net>
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
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include "stubinfo.h"
#include "dpmiwrp.h"
#include "dos.h"
#include "dpmiwrp.h"
#include "coff.h"
#include "elfp.h"
#include "util.h"
#include "stub_priv.h"
#include "djdev64/dj64init.h"
#include "djdev64/stub.h"

#define DJSTUB_VERSION 7
#define DJSTUB_API_VERSION 5

#define STUB_DEBUG 1
#if STUB_DEBUG
#define stub_debug(...) J_printf(do_printf, DJ64_PRINT_LOG, __VA_ARGS__)
#else
#define stub_debug(...)
#endif

#define FLG1_OFF STFLAGS_OFF
#define FLG2_OFF (STFLAGS_OFF + 1)

#define STFLG1_STATIC  0x40  // static linking
/* 2 flags below are chosen for compatibility between v4 and v5 stubs.
 * They can't be set together, and as such, when we have a core payload,
 * there is no indication of whether or not the user payload is also
 * present. */
#define STFLG1_NO32PL  0x80  // no 32bit payload
#define STFLG2_C32PL   0x40  // have core 32bit payload
#define STFLG2_EMBOV   0x80  // embedded overlay layout

#define MB (1024 * 1024)
#define VA_SZ (2*MB)

typedef struct
{
    uint32_t offset32;
    unsigned short selector;
} DPMI_FP;

typedef struct
{
    uint16_t rm;
    int pm;
} dpmi_dos_block;

static DPMI_FP clnt_entry;

static const char *_basename(const char *name)
{
    const char *p;
    p = strrchr(name, '\\');
    if (!p)
        p = name;
    else
        p++;
    return p;
}

#if 0
static char *_fname(char *name)
{
    char *p, *p1;
    p = strrchr(name, '\\');
    if (!p)
        p = name;
    else
        p++;
    p1 = strrchr(p, '.');
    if (p1)
        p1[0] = '\0';
    return p;
}
#endif

static unsigned _host_open(const char *pathname, unsigned mode, int *handle)
{
    int fd = open(pathname, mode | O_CLOEXEC);
    if (fd == -1)
        return 1;
    *handle = fd;
    return 0;
}

static unsigned _host_read(int handle, void *buffer, unsigned count,
        unsigned *numread)
{
    int rd = read(handle, buffer, count);
    if (rd == -1)
        return 1;
    *numread = rd;
    return 0;
}

static unsigned long _host_seek(int handle, unsigned long offset, int origin)
{
    return lseek(handle, offset, origin);
}

static int _host_close(int handle)
{
    return close(handle);
}

static struct dos_ops hops = {
    ._dos_open = _host_open,
    ._dos_read = _host_read,
    ._dos_seek = _host_seek,
    ._dos_close = _host_close,
};

__attribute__((format(printf, 3, 4)))
static void J_printf(void (*do_printf)(int prio, const char *fmt, va_list ap),
    int prio, const char *fmt, ...)
{
    va_list val;

    va_start(val, fmt);
    do_printf(prio, fmt, val);
    va_end(val);
}

#define exit(x) return -(x)
#define error(...) J_printf(do_printf, DJ64_PRINT_TERMINAL, __VA_ARGS__)
int djstub_main(int argc, char *argv[], char *envp[],
    unsigned psp_sel, int ifile, int ver,
    struct stub_ret_regs *regs, char *(*lin2ptr)(unsigned lin),
    struct dos_ops *dosops, struct dpmi_ops *dpmiops,
    void (*do_printf)(int prio, const char *fmt, va_list ap),
    int (*uput)(int))
{
    int pfile = -1;
    off_t coffset = 0;
    uint32_t coffsize = 0;
    uint32_t noffset = 0;
    uint32_t nsize = 0;
    uint32_t noffset2 = 0;
    uint32_t nsize2 = 0;
    int rc, i;
#define BUF_SIZE 0x40
    char buf[BUF_SIZE];
    int done = 0;
    int dyn = 0;
    int pl32 = 0;
    uint32_t va;
    uint32_t va_size;
    uint32_t mem_lin;
    uint32_t mem_base;
    unsigned short clnt_ds;
    unsigned short stubinfo_fs;
    __dpmi_meminfo info;
    dpmi_dos_block db;
    void *handle;
    _GO32_StubInfo stubinfo = {0};
    _GO32_StubInfo *stubinfo_p;
    struct ldops *ops = NULL;
    int STFLAGS_OFF = 0x2c;
    uint8_t stub_ver = 0;
#define BARE_STUB() (stub_ver == 0)

    if (ver == 0) {
        /* backward-compat code */
        stub_debug("Opening self at %s\n", argv[0]);
        rc = dosops->_dos_open(argv[0], O_RDONLY, &ifile);
        if (rc) {
            error("cannot open %s\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        ver = DJSTUB_API_VERSION;
    }
    if (ver != DJSTUB_API_VERSION) {
        error("Version mismatch: want %i got %i\n", DJSTUB_API_VERSION, ver);
        exit(1);
    }

    register_dpmiops(dpmiops);
    stubinfo.cpl_fd = -1;
    for (i = 0; envp[i]; i++) {
        const char *s = "ELFLOAD=";
        int l = strlen(s);
        if (strncmp(envp[i], s, l) == 0) {
            pfile = open(CRT0, O_RDONLY | O_CLOEXEC);
            if (pfile == -1)
                return -1;
            stubinfo.cpl_fd = uput(pfile);
            stubinfo.elfload_arg = atoi(envp[i] + l);
            ops = &elf_ops;
            dyn = 1;
            done = 1;
        }
    }
    while (!done) {
        unsigned rd;
#if STUB_DEBUG
        int cnt = 0;
#endif

        stub_debug("Expecting header at 0x%lx\n", coffset);
        rc = dosops->_dos_read(ifile, buf, BUF_SIZE, &rd);
        if (rc) {
            error("stub: read() failure\n");
            exit(EXIT_FAILURE);
        }
        if (rd != BUF_SIZE) {
            error("stub: read(%i)=%i, wrong exe file\n", BUF_SIZE, rd);
            exit(EXIT_FAILURE);
        }
        if (buf[0] == 'M' && buf[1] == 'Z' && buf[8] == 4 && buf[9] == 0) {
            /* lfanew */
            uint32_t offs;
            int moff = 0;

            stub_ver = buf[0x3b];
#if STUB_DEBUG
            cnt++;
#endif
            stubinfo.stubinfo_ver |= stub_ver << 16;
            if (stub_ver >= 6)
                STFLAGS_OFF = 0x38;
            stub_debug("Found exe header %i at 0x%lx\n", cnt, coffset);
            memcpy(&offs, &buf[0x3c], sizeof(offs));
            /* fixup for old stubs: if they have any 32bit payload, that
             * always includes the core payload. */
            if (stub_ver > 0 && stub_ver < 5 && !(buf[FLG1_OFF] & STFLG1_NO32PL))
                buf[FLG2_OFF] |= STFLG2_C32PL;

            if (!(buf[FLG2_OFF] & STFLG2_C32PL)) {
                dyn++;
                pfile = open(CRT0, O_RDONLY | O_CLOEXEC);
                if (pfile == -1)
                    return -1;
                stubinfo.cpl_fd = uput(pfile);
                ops = &elf_ops;
            } else {
                pfile = ifile;
            }
            if (buf[FLG1_OFF] & STFLG1_NO32PL) {
                noffset = offs;
                moff = 4;
                done = 1;
            } else {
                pl32++;
                coffset = offs;
                if (stub_ver >= 6) {
                    uint32_t ooffs;
                    memcpy(&ooffs, &buf[0x2c], sizeof(ooffs));
                    coffset += ooffs;
                }
                memcpy(&coffsize, &buf[0x1c], sizeof(coffsize));
                noffset = offs;
                if (stub_ver < 6 || !(buf[FLG2_OFF] & STFLG2_EMBOV))
                    noffset += coffsize;
            }
            memcpy(&nsize, &buf[0x20 - moff], sizeof(nsize));
            if (nsize)
                noffset2 = noffset + nsize;
            memcpy(&nsize2, &buf[0x24 - moff], sizeof(nsize2));
            memcpy(&stubinfo.flags, &buf[STFLAGS_OFF], 2);
            if (stub_ver >= 6) {
                uint32_t nmoffs;
                memcpy(&nmoffs, &buf[0x28], sizeof(nmoffs));
                if (nmoffs) {
                    dosops->_dos_seek(ifile, noffset + nmoffs, SEEK_SET);
                    rc = dosops->_dos_read(ifile, stubinfo.payload2_name,
                            16, &rd);
                    stubinfo.payload2_name[rd] = '\0';
                }
            } else if (stub_ver >= 4) {
                strncpy(stubinfo.payload2_name, &buf[0x2e], 12);
                stubinfo.payload2_name[12] = '\0';
                strcat(stubinfo.payload2_name, ".dbg");
            } else if (BARE_STUB()) {
                done = 1;
                ops = &elf_ops;
                assert(dyn && pl32);
                pl32 = 0;
            } else {
                error("unsupported stub version %i\n", stub_ver);
                return -1;
            }
            stub_debug("suppl name %s\n", stubinfo.payload2_name[0] ?
                    stubinfo.payload2_name : "<not set>");
        } else if (buf[0] == 0x4c && buf[1] == 0x01) { /* it's a COFF */
            done = 1;
            ops = &coff_ops;
        } else if (buf[0] == 0x7f && buf[1] == 0x45 &&
                buf[2] == 0x4c && buf[3] == 0x46) { /* it's an ELF */
            done = 1;
            ops = &elf_ops;
        } else {
            error("not an exe %s at %lx\n", argv[0], coffset);
            exit(EXIT_FAILURE);
        }
        dosops->_dos_seek(ifile, coffset, SEEK_SET);
    }
    assert(ops);

    strncpy(stubinfo.magic, "dj64 (C) stsp", sizeof(stubinfo.magic));
    stubinfo.size = sizeof(stubinfo);
    i = 0;
    while(*envp) {
        i += strlen(*envp) + 1;
        envp++;
    }
    if (i) {
        i += strlen(argv[0]) + 1;
        i += 3;
    }
    stub_debug("env size %i\n", i);
    stubinfo.env_size = i;
    stubinfo.minstack = 0x80000;
    stubinfo.minkeep = 0x4000;
    strncpy(stubinfo.argv0, _basename(argv[0]), sizeof(stubinfo.argv0));
    stubinfo.argv0[sizeof(stubinfo.argv0) - 1] = '\0';
    /* basename seems unused and produces warning about missing 0-terminator */
//    strncpy(stubinfo.basename, _fname(argv0), sizeof(stubinfo.basename));
//    stubinfo.basename[sizeof(stubinfo.basename) - 1] = '\0';
    strncpy(stubinfo.dpmi_server, "CWSDPMI.EXE", sizeof(stubinfo.dpmi_server));
#define max(a, b) ((a) > (b) ? (a) : (b))
    stubinfo.psp_selector = psp_sel;
    /* DJGPP relies on ds_selector, cs_selector and ds_segment all mapping
     * the same real-mode memory block. */
    dosops->_dos_link_umb(1);
    db.rm = __dpmi_allocate_dos_memory(stubinfo.minkeep >> 4, &db.pm);
    dosops->_dos_link_umb(0);
    stub_debug("rm seg 0x%x\n", db.rm);
    stubinfo.ds_selector = db.pm;
    stubinfo.ds_segment = db.rm;
    /* create alias */
    stubinfo.cs_selector = __dpmi_create_alias_descriptor(db.pm);
    /* set descriptor access rights */
    __dpmi_set_descriptor_access_rights(stubinfo.cs_selector, 0x00fb);

    clnt_entry.selector = __dpmi_allocate_ldt_descriptors(1);
    clnt_ds = __dpmi_allocate_ldt_descriptors(1);

    register_dosops(dyn ? &hops : dosops);
    handle = ops->read_headers(pfile);
    if (!handle)
        exit(EXIT_FAILURE);
    va = ops->get_va(handle);
    va_size = ops->get_length(handle);
    clnt_entry.offset32 = ops->get_entry(handle);
    stub_debug("va 0x%x va_size 0x%x entry 0x%x\n",
            va, va_size, clnt_entry.offset32);
    if (va_size > MB)
        exit(EXIT_FAILURE);
    /* if we load 2 payloads, use larger estimate */
    if (dyn && (pl32 || BARE_STUB())) {
        stubinfo.initial_size = VA_SZ;
        stubinfo.upl_base = va + MB;
        stubinfo.upl_size = VA_SZ - MB;
    } else {
        stubinfo.initial_size = max(va_size, 0x10000);
    }
    info.size = PAGE_ALIGN(stubinfo.initial_size);
    /* allocate mem */
    __dpmi_allocate_memory(&info);
    stubinfo.memory_handle = info.handle;
    mem_lin = info.address;
    mem_base = mem_lin - va;
    stubinfo.mem_base = mem_base;
    stub_debug("mem_lin 0x%x mem_base 0x%x\n", mem_lin, mem_base);
    ops->read_sections(handle, lin2ptr(mem_base), pfile, dyn ? 0 : coffset);
    ops->close(handle);
    unregister_dosops();
    if (dyn && pl32) {
        uint32_t va2;
        uint32_t va_size2;

        /* dyn loaded, now pl32 */
        register_dosops(dosops);
        handle = ops->read_headers(ifile);
        if (!handle)
            exit(EXIT_FAILURE);
        stubinfo.uentry = ops->get_entry(handle);
        va2 = ops->get_va(handle);
        va_size2 = ops->get_length(handle);
        stub_debug("va 0x%x va_size 0x%x\n", va2, va_size2);
        if (va_size2 > MB)
            exit(EXIT_FAILURE);
        if (va2 < va + va_size || va2 + va_size2 - va > VA_SZ)
            exit(EXIT_FAILURE);
        ops->read_sections(handle, lin2ptr(mem_base), ifile, coffset);
        ops->close(handle);
        unregister_dosops();
    }

    /* set base */
    __dpmi_set_segment_base_address(clnt_entry.selector, mem_base);
    /* set descriptor access rights */
    __dpmi_set_descriptor_access_rights(clnt_entry.selector, 0xc0fb);
    /* set limit */
    __dpmi_set_segment_limit(clnt_entry.selector, 0xffffffff);
    /* set base */
    __dpmi_set_segment_base_address(clnt_ds, mem_base);
    /* set descriptor access rights */
    __dpmi_set_descriptor_access_rights(clnt_ds, 0xc0f3);
    /* set limit */
    __dpmi_set_segment_limit(clnt_ds, 0xffffffff);

    stubinfo_fs = __dpmi_allocate_ldt_descriptors(1);
    info.size = PAGE_ALIGN(sizeof(_GO32_StubInfo));
    __dpmi_allocate_memory(&info);
    __dpmi_set_segment_base_address(stubinfo_fs, info.address);
    __dpmi_set_segment_limit(stubinfo_fs, sizeof(_GO32_StubInfo) - 1);
    stubinfo_p = (_GO32_StubInfo *)lin2ptr(info.address);

    stubinfo.self_fd = ifile;
    stubinfo.self_offs = coffset;
    stubinfo.self_size = coffsize;
    stubinfo.payload_offs = noffset;
    stubinfo.payload_size = nsize;
    stubinfo.payload2_offs = noffset2;
    stubinfo.payload2_size = nsize2;
    dosops->_dos_seek(ifile, noffset, SEEK_SET);
    if (nsize > 0)
        stub_debug("Found payload of size %i at 0x%x\n", nsize, noffset);
    stubinfo.stubinfo_ver |= DJSTUB_VERSION;

    unregister_dpmiops();

    memcpy(stubinfo_p, &stubinfo, sizeof(stubinfo));
    stub_debug("Jump to entry...\n");
    regs->fs = stubinfo_fs;
    regs->ds = clnt_ds;
    regs->cs = clnt_entry.selector;
    regs->eip = clnt_entry.offset32;
    return 0;
}
