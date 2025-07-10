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
#define _GNU_SOURCE
#include <stdio.h>
#include <setjmp.h>
#include <dlfcn.h>
#include "djdev64/djdev64.h"
#include "elf_priv.h"

#define DJ64_ELFEXEC_VERSION 2

struct exec_info {
    /* volatile because of longjmp() vs auto storage */
    /* unsigned char to not clash with go64/dj64 errors */
    volatile unsigned char exit_code;
    jmp_buf exit_jmp;
};

struct exec_handle {
    void *dlobj;
    int (*m)(int, char **);
    int (*ae2)(void(*)(void*,int),void*);
    char *estart;
    char *eend;
};
static struct exec_handle ehands[1];  // expand if more than 1 ever needed

static void exit_exec(void *handle, int rc);

int djdev64_exec(const char *path, int handle, int libid, unsigned flags)
{
    struct exec_handle *eh = &ehands[0];
    int rtld_flags = RTLD_LOCAL | RTLD_NOW;
    dj64init2_t *i2;

    eh->dlobj = NULL;
#if HAVE_DECL_RTLD_DEEPBIND
    rtld_flags |= RTLD_DEEPBIND;
#endif
    eh->dlobj = dlopen(path, rtld_flags);
    if (!eh->dlobj) {
        printf("error loading %s: %s\n", path, dlerror());
        return -1;
    }
    eh->estart = dlsym(eh->dlobj, "_binary_tmp_o_elf_start");
    eh->eend = dlsym(eh->dlobj, "_binary_tmp_o_elf_end");
    eh->m = dlsym(eh->dlobj, "main");
    if (!eh->m) {
        printf("error: can't find \"main\"\n");
        goto out;
    }
    eh->ae2 = dlsym(eh->dlobj, "atexit2");
    if (!eh->ae2) {
        printf("error: can't find \"atexit2\"\n");
        goto out;
    }
    i2 = dlsym(eh->dlobj, "dj64init2");
    if (!i2) {
        printf("error: can't find \"dj64init2\"\n");
        goto out;
    }

    i2(handle, libid);
    return 0;
out:
    dlclose(eh->dlobj);
    return -1;
}

int djelf64_exec_self(void)
{
#ifdef RTLD_DEFAULT
    struct exec_handle *eh = &ehands[0];
    void **dlh = dlsym(RTLD_DEFAULT, "dj64_dl_handle_self");
    int *ee_ver = dlsym(RTLD_DEFAULT, "dj64_elfexec_version");

    if (!dlh || !ee_ver)
        return -1;
    if (*ee_ver != DJ64_ELFEXEC_VERSION) {
        fprintf(stderr, "wrong elfexec version, want %i got %i\n",
                DJ64_ELFEXEC_VERSION, *ee_ver);
    }
    eh->dlobj = NULL;
    eh->estart = dlsym(RTLD_DEFAULT, "_binary_tmp_o_elf_start");
    eh->eend = dlsym(RTLD_DEFAULT, "_binary_tmp_o_elf_end");
    eh->m = dlsym(RTLD_DEFAULT, "dj64_startup_hook");
    if (!eh->m) {
        printf("error: can't find \"dj64_startup_hook\"\n");
        return -1;
    }
    eh->ae2 = dlsym(*dlh, "atexit2");
    if (!eh->ae2) {
        printf("error: can't find \"atexit2\"\n");
        return -1;
    }
    return 0;
#else
    return -1;
#endif
}

char *djelf64_parse(int eid, uint32_t *r_size)
{
    struct exec_handle *eh;

    if (eid != 0)
        return NULL;
    eh = &ehands[eid];
    *r_size = eh->eend - eh->estart;
    return eh->estart;
}

int djelf64_run(int eid, int argc, char **argv)
{
    struct exec_handle *eh;
    struct exec_info ei;
    int ret = -1, rc;

    if (eid != 0)
        return -1;
    eh = &ehands[eid];
    rc = eh->ae2(exit_exec, &ei);
    if (rc == -1) {
        printf("error: atexit2() failed\n");
        goto out;
    }
    if (setjmp(ei.exit_jmp))
        ret = ei.exit_code;
    else
        ret = (unsigned char)eh->m(argc, argv);
    eh->ae2(NULL, NULL);
out:
    if (eh->dlobj)
        dlclose(eh->dlobj);
    return ret;
}

static void exit_exec(void *handle, int rc)
{
    struct exec_info *ei = handle;
    ei->exit_code = rc;
    longjmp(ei->exit_jmp, 1);
}
